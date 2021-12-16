/* Copyright [2019] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/15.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */

#include <srt/srt.h>
#include <srt/udt.h>

#include <string>
#include <vector>

#include "st_srt.hpp"
#include "log/log.hpp"
#include "st_tcp.hpp"

static StSrtMode default_srt_mode = ST_SRT_MODE_FILE;

int st_srt_epoll(SRTSOCKET fd, StSrtEvent evt, utime_t tm, std::string& err) {
    int ret = SrtStDispatch::get_instance()->st_srt_epoll(fd, evt, tm, err);

    if (ret != 0) {
        tmss_error("srt epoll failed fd:{}, ret:{}, err:{}", fd, ret, err);
        return ret;
    }

    return ret;
}

int st_srt_socket_error(SRT_SOCKSTATUS status) {
    switch (status) {
        case SRTS_BROKEN:
            return error_srt_socket_broken;
        case SRTS_CLOSING:
            return error_srt_socket_closing;
        case SRTS_CLOSED:
            return error_srt_socket_already_closed;
        case SRTS_NONEXIST:
            return error_srt_socket_noexist;
        default:
            return error_srt_socket_other;
    }

    return 0;
}

int st_srt_set_global_mode(StSrtMode mode) {
    if (mode == ST_SRT_MODE_LIVE) {
        default_srt_mode = ST_SRT_MODE_LIVE;
    } else {
        default_srt_mode = ST_SRT_MODE_FILE;
    }

    return 0;
}

SRTSOCKET st_srt_create_fd(StSrtMode mode) {
    auto fd      = srt_create_socket();
    bool syn     = false;
    bool drop    = false;
    bool tspd    = false;

    if (fd == SRT_INVALID_SOCK) {
        tmss_error("[st_srt_create]: srt create socket  failed");
        return fd;
    }

    srt_setsockflag(fd, SRTO_RCVSYN, &syn, sizeof(syn));
    srt_setsockflag(fd, SRTO_SNDSYN, &syn, sizeof(syn));

    // set default mode
    if (mode == ST_SRT_MODE_GLOBAL) {
        mode = default_srt_mode;
    }

    if (mode == ST_SRT_MODE_FILE) {
        SRT_TRANSTYPE t = SRTT_FILE;
        srt_setsockflag(fd, SRTO_TRANSTYPE, &t, sizeof(t));

        linger lin;
        lin.l_linger = 2;  // 2 seconds for file
        lin.l_onoff = 1;
        srt_setsockopt(fd, 0, SRTO_LINGER, &lin, sizeof(lin));

        // rapid nak
        bool nak = true;
        srt_setsockflag(fd, SRTO_NAKREPORT, &nak, sizeof(nak));
    } else {
        SRT_TRANSTYPE t = SRTT_LIVE;
        srt_setsockflag(fd, SRTO_TRANSTYPE, &t, sizeof(t));

        // error not support
        // bool api = false;
        // srt_setsockflag(fd, SRTO_MESSAGEAPI, &api, sizeof(api));

        // bool nak = false;
        // srt_setsockflag(fd, SRTO_NAKREPORT, &nak, sizeof(nak));

        // int pay_size = 0;
        // srt_setsockflag(fd, SRTO_PAYLOADSIZE, &pay_size, sizeof(pay_size));

        int send_delay = -1;
        srt_setsockflag(fd, SRTO_TSBPDMODE, &tspd, sizeof(tspd));
        srt_setsockflag(fd, SRTO_SNDDROPDELAY, &send_delay, sizeof(send_delay));
        srt_setsockflag(fd, SRTO_TLPKTDROP, &drop, sizeof(drop));

        linger lin;
        lin.l_linger = 3;  // 3 seconds for live cc
        lin.l_onoff = 0;
        srt_setsockopt(fd, 0, SRTO_LINGER, &lin, sizeof(lin));
    }

    return fd;
}

error_t  st_srt_connect(SRTSOCKET fd, const std::string& server,
                        int port, utime_t tm, UDPSOCKET ufd, bool one_rtt) {
    struct sockaddr_in target_addr{};

    target_addr.sin_family = AF_INET;
    target_addr.sin_port   = htons(port);
    int ret                = error_success;

    std::string server_ip  = dns_resolve(server);

    if (server_ip.empty()) {
        tmss_error("st_srt_connect: dns_resolve addr failed, {}:{}", server, port);
        return error_system_ip_invalid;
    }

    if (inet_pton(AF_INET, server_ip.data(), &target_addr.sin_addr) != 1) {
        tmss_error("st_srt_connect: srt create socket addr failed, {}:{}", server, port);
        return error_srt_socket_create;
    }

    if (ufd > 0) {
        if ((ret = srt_bind_acquire(fd, ufd)) == SRT_ERROR) {
            tmss_error("[srt_bind_acquire] failed ret:{}", ret);
            return error_srt_socket_bind;
        }
    }

    bool one_rtt_ok = false;

    ret = srt_connect(fd, (struct sockaddr*) &target_addr, sizeof(target_addr));


    if (ret == SRT_ERROR) {
        tmss_error("[st_srt_connect]: srt create socket addr failed, {}:{}", server, port);
        return error_srt_socket_connect;
    }

    std::string err;
    ret = st_srt_epoll(fd, ST_SRT_CONN, tm, err);

    if (ret != 0) {
        tmss_error("[st_srt_connect]: srt create socket addr failed, {}:{}, "
                   "one_rtt_ok:{}, one_rtt_conf:{}",
                   server, port, one_rtt_ok, one_rtt);
        return ret;
    }

    tmss_info("[st_srt_connect]: srt create socket addr failed, {}:{}, "
              "one_rtt_ok:{}, one_rtt_conf:{}",
              server, port, one_rtt_ok, one_rtt);

    return ret;
}

error_t st_srt_connect(SRTSOCKET fd, const std::string& server, int port,
                       const std::string& self_ip, int self_port, utime_t tm,
                       bool one_rtt) {
    struct sockaddr_in target_addr{};
    target_addr.sin_family = AF_INET;
    target_addr.sin_port   = htons(port);

    std::string server_ip  = dns_resolve(server);

    if (server_ip.empty()) {
        tmss_error("st_srt_connect: dns_resolve addr failed, {}:{}", server, port);
        return error_system_ip_invalid;
    }

    if (inet_pton(AF_INET, server_ip.data(), &target_addr.sin_addr) != 1) {
        tmss_error("st_srt_connect: srt create socket addr failed, {}:{}", server, port);
        return error_srt_socket_create;
    }

    struct sockaddr_in self_addr{};
    self_addr.sin_family = AF_INET;
    self_addr.sin_port   = htons(self_port);

    if (inet_pton(AF_INET, self_ip.data(), &self_addr.sin_addr) != 1) {
        tmss_error("st_srt_connect: srt create socket addr failed, {}:{}", self_ip, self_port);
        return error_srt_socket_create;
    }

    int ret = error_success;

    bool one_rtt_ok = false;

    ret = srt_connect_bind(fd, (const struct sockaddr*) &self_addr,
            (const struct sockaddr*) &target_addr, sizeof(self_addr));

    if (ret == SRT_ERROR) {
        tmss_error("[st_srt_connect2]: srt create socket addr failed, target:"
                   " {}:{}, self:{}:{}, one_rtt_ok:{},"
                   "one_rtt_conf:{} ",
                   server, port, self_ip, self_port, one_rtt_ok, one_rtt);
        return error_srt_socket_connect;
    }

    std::string err;
    ret = st_srt_epoll(fd, ST_SRT_CONN, tm, err);

    if (ret != 0) {
        tmss_error("[st_srt_connect2]: srt connect socket addr failed, server:{}, port:{}, "
                   "ret:{}, err:{}", server, port, ret, err);
        return ret;
    }

    tmss_info("[st_srt_connect2]: srt create socket addr failed, target: "
              "{}:{}, self:{}:{}, one_rtt_ok:{},"
              "one_rtt_conf:{} ",
              server, port, self_ip, self_port, one_rtt_ok, one_rtt);

    return ret;
}

extern error_t st_tcp_fd_reuseport(int fd);

error_t st_srt_listen(SRTSOCKET fd, const std::string& server, int port,
                      int back_log, bool reuse_port) {
    struct sockaddr_in self_addr{};

    self_addr.sin_family = AF_INET;
    self_addr.sin_port   = htons(port);

    do {
        UDPSOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (sock == -1) {
            tmss_error("udp socket create failed sock:{}", sock);
            goto failed;
        }

        if (reuse_port) {
            if (st_tcp_fd_reuseaddr(sock) != error_success) {
                tmss_error("reuse addr failed port:{}", server);
                goto failed;
            }

            if (st_tcp_fd_reuseport(sock) != error_success) {
                tmss_error("reuse port failed port:{}", port);
                goto failed;
            }
        }

        if (::bind(sock, reinterpret_cast<struct sockaddr*>(&self_addr),
                sizeof(self_addr)) == -1) {
            tmss_error("udp socket create failed sock:{}", sock);
            goto failed;
        }

        if (srt_bind_acquire(fd, sock) == SRT_ERROR) {
            tmss_error("[srt_bind]: socket addr failed, server:{}, port:{}", server, port);
            goto failed;
        }

        break;
failed:
        close(sock);

        if (reuse_port) {
            return error_srt_socket_create;
        }

        if (srt_bind(fd, reinterpret_cast<struct sockaddr*>(&self_addr),
                sizeof(self_addr) == SRT_ERROR)) {
            tmss_error("[srt_bind]: socket addr failed, server:{}, port:{}", server, port);
            return error_srt_socket_bind;
        }
    } while (0);

    if (srt_listen(fd, back_log) == SRT_ERROR) {
        tmss_error("[srt_listen]: socket addr failed, server:{}, port:{}", server, port);
        return error_srt_socket_listen;
    }

    return error_success;
}

SRTSOCKET st_srt_accept(SRTSOCKET fd, struct sockaddr *addr, int *addrlen, utime_t timeout) {
    std::string err;
    SRTSOCKET ret = SRT_INVALID_SOCK;

    while ((ret = srt_accept(fd, addr, addrlen)) == SRT_INVALID_SOCK) {
        int eno = 0;

        if (st_srt_again(eno)) {
            ret = st_srt_epoll(fd, ST_SRT_ACCEPT, timeout, err);

            if (ret != error_success) {
                tmss_error("[st_srt_accept]: srt epoll failed, fd:{}, "
                           "ret:{}, err:{}", fd, ret, err);
                return ret;
            }
        } else {
            tmss_error("[st_srt_accept] failed, fd:{}, ret:{}, err:{}", fd, ret, err);
            return ret;
        }
    }
    return ret;
}

error_t st_srt_read(SRTSOCKET fd, void *buf, size_t nbyte, utime_t timeout, size_t& nread) {
    std::string err;
    int ret = error_success;

    while ((ret = srt_recv(fd, reinterpret_cast<char*>(buf), nbyte)) == SRT_ERROR) {
        int eno = 0;
        if (st_srt_again(eno)) {
            ret = st_srt_epoll(fd, ST_SRT_IN, timeout, err);
            if (ret != error_success) {
                tmss_error("[st_srt_read]: srt read epoll failed, fd:{}, "
                           "ret:{}, err:{}", fd, ret, err);
                return ret;
            }
        } else {
            err = srt_getlasterror_str();
            tmss_error("[st_srt_read]: srt read socket addr failed, fd:{}, "
                       "srt_errno:{}, ret:{}, err:{}", fd, eno, ret, err);
            return ret;
        }
    }

    if (ret <= 0) {
        err = srt_getlasterror_str();
        tmss_error("[st_srt_read]: srt read socket addr failed, fd:{}, "
                   "ret:{}, err:{}", fd, ret, err);
        return ret == 0 ? error_srt_socket_already_closed : error_srt_socket_other;
    }

    nread = ret;
    return error_success;
}

error_t st_srt_write(SRTSOCKET fd, const void *buf, size_t nbyte, utime_t timeout, size_t& nwrite) {
    std::string err;
    int ret = error_success;

    size_t has_send = 0;

    // srt 的写限制
    while (has_send < nbyte) {
        size_t next_send = nbyte - has_send;

        if (next_send > SRT_LIVE_DEF_PLSIZE) {
            next_send = SRT_LIVE_DEF_PLSIZE;
        }

        while ((ret = srt_sendmsg(fd, reinterpret_cast<const char*>(buf) + has_send, next_send,
                 -1, true)) == SRT_ERROR) {
            int eno = 0;
            if (st_srt_again(eno)) {
                ret = st_srt_epoll(fd, ST_SRT_OUT, timeout, err);
                if (ret != error_success) {
                    tmss_error("[st_srt_write]: srt write epoll failed, fd:{}, "
                               "ret:{}, err:{}", fd, ret, err);
                    return ret;
                }
            } else {
                err = srt_getlasterror_str();
                tmss_error("[st_srt_write]: srt write socket addr failed, fd:{},"
                           " errno:{}, err:{}", fd, eno, err);
                return ret;
            }
        }
        has_send += ret;
    }
    nwrite = has_send;
    return error_success;
}

error_t st_srt_close(SRTSOCKET& fd) {
    // TODO(weideng): FIXME HERE WITH LINGER
    // 延迟关闭, srt需要提供是否写完了fd的配置
    // st_usleep(3 * 1000 * 1000);
    srt_close(fd);
    tmss_info("close srt fd:{}", fd);
    fd = SRT_INVALID_SOCK;
    return error_success;
}

bool st_srt_again(int &err) {
    err = srt_getlasterror(nullptr);
    return err / 1000 == MJ_AGAIN;
}

int st_srt_stat(SRTSOCKET fd, std::string& res, SRT_TRACEBSTATS* perf) {
    int ret = srt_bstats(fd, perf, 1);
    if (ret == SRT_ERROR) {
        return ret;
    }

    return st_srt_stat_print(res, perf);
}

int st_srt_stat_print(std::string& res, SRT_TRACEBSTATS* perf) {
    std::stringstream in;

    // 重传率
    double re_send = st_srt_stat_retrans(perf, true);
    double re_rcv  = st_srt_stat_retrans(perf, false);

    // 丢包率
    double send_loss = st_srt_stat_loss(perf, true);
    double recv_loss = st_srt_stat_loss(perf, false);

    in  << "rcv_drop_bytes: "  <<  perf->byteRcvDrop << "\t"
        << "send_loss_bytes: " <<  perf->byteSndDrop << "\t"
        << "send_re: "         <<  re_send   << "\t"
        << "rcv_re: "          <<  re_rcv    << "\t"
        << "send_loss: "       <<  send_loss << "\t"
        << "recv_loss: "       <<  recv_loss << "\t"
        << "send_app_bytes: "  <<  perf->byteSentUnique << "\t"
        << "rcv_app_bytes: "   <<  perf->byteRecvUnique << "\t"
        << "send_total_bytes: " <<  perf->byteSent << "\t"
        << "rcv_total_bytes: "  <<  perf->byteRecv;

    res =  in.str();
    return error_success;
}

double st_srt_stat_loss(SRT_TRACEBSTATS* perf, bool is_send) {
    if (is_send) {
        return (perf->byteSentUnique > 0) ?
            perf->byteSndDrop * 10000.0 / perf->byteSentUnique : 0.0;
    } else {
        return (perf->byteRecvUnique > 0) ?
            perf->byteRcvDrop * 10000.0 / perf->byteRecvUnique : 0.0;
    }
}

double st_srt_stat_retrans(SRT_TRACEBSTATS* perf, bool is_send) {
    if (is_send) {
        return (perf->byteSentUnique > 0) ?
            (perf->byteSent - perf->byteSentUnique) * 10000.0 / perf->byteSentUnique : 0.0;
    } else {
        return (perf->byteRecvUnique > 0) ?
            (perf->byteRecv - perf->byteRecvUnique) * 10000.0 / perf->byteRecvUnique : 0.0;
    }
}

SrtEventCondition::SrtEventCondition(StSrtEvent evt) {
    cond        = st_cond_new();
    event       = evt;
    err         = 0;
}

SrtEventCondition::~SrtEventCondition() {
    st_cond_destroy(cond);
}

SrtStDispatch* SrtStDispatch::get_instance() {
    thread_local SrtStDispatch* srt_dispatch = nullptr;
    if (srt_dispatch == nullptr) {
        srt_dispatch = new SrtStDispatch();
        srt_dispatch->init();
    }
    return srt_dispatch;
}

SrtStDispatch::SrtStDispatch() : ICoroutineHandler("srt-dispatch") {
    cid        = get_ctx_id();
    stop       = false;
    // coroutine  = new STCoroutine(std::string("srt-dispatch"), this, cid);
    srt_eid    = -1;
}

SrtStDispatch::~SrtStDispatch() {
    // delete coroutine;
}

int SrtStDispatch::init() {
    srt_eid = srt_epoll_create();

    srt_epoll_set(srt_eid, SRT_EPOLL_ENABLE_EMPTY);
    return start();
}

int SrtStDispatch::st_srt_epoll(SRTSOCKET fd, StSrtEvent event, utime_t timeout, std::string& err) {
    int evt = 0;

    auto it  = conditions.find(fd);
    bool add;
    int  ret = error_success;

    if (it == conditions.end()) {
        add   = true;
        evt   = event | ST_SRT_ERR;
    } else {
        int old_evt = get_exits_evt(fd);
        add         = false;
        evt         = old_evt | event | ST_SRT_ERR;
    }

    if (add) {
        if (UDT::epoll_add_usock(srt_eid, fd, &evt) == SRT_ERROR) {
            return SRT_ERROR;
        }
    } else {
        evt |= SRT_EPOLL_ETONLY;  // et only
        if (UDT::epoll_update_usock(srt_eid, fd, &evt) == SRT_ERROR) {
            return SRT_ERROR;
        }
    }

    // st wait util timeout or notify
    SrtEventCondition cnd(event);
    conditions[fd].insert(&cnd);
    ret = st_cond_timedwait(cnd.cond, timeout);

    if (conditions.count(fd))
        conditions[fd].erase(&cnd);

    int old_evt = get_exits_evt(fd);

    if (old_evt == 0) {
        // remove all event when nothing happend
        if (UDT::epoll_remove_usock(srt_eid, fd) == SRT_ERROR) {
            tmss_error("remove fd failed ingore it fd:{}", fd);
        }
    } else {
        old_evt |= SRT_EPOLL_ETONLY;
        // try remove self evt
        if (UDT::epoll_update_usock(srt_eid, fd, &old_evt) == SRT_ERROR) {
            tmss_error("update fd failed ingore it fd:{}", fd);
        }
    }

    if (cnd.err) {
        err = cnd.err_msg;
        return cnd.err;
    }

    if (ret != 0) {
        if (errno == ETIME) {
            return error_srt_socket_timeout;
        } else if (errno == EINTR) {
            return error_srt_socket_eintr;
        }
        return error_srt_socket_other;
    }

    return ret;
}

int SrtStDispatch::get_exits_evt(SRTSOCKET fd) {
    int old_evt = 0;
    auto it  = conditions.find(fd);

    if (it == conditions.end()) {
        return old_evt;
    }

    for (auto e : it->second) {
        old_evt             |=  e->event;
    }
    return old_evt;
}

error_t SrtStDispatch::cycle() {
    const int len = 1024;
    std::vector<SRT_EPOLL_EVENT> events(len);
    while (!stop) {
        int es = UDT::epoll_uwait(srt_eid, &events[0], len, 0);

        for (int i = 0; i < es; ++i) {
            SRT_EPOLL_EVENT& event = events[i];
            auto it = conditions.find(event.fd);

            // empty event removed from events
            if (it == conditions.end()) {
                tmss_error("unknow srt_fd:{}", event.fd);
                UDT::epoll_remove_ssock(srt_eid, event.fd);
            } else {
                std::set<SrtEventCondition*>& sets = it->second;
                int  err = 0;
                const char* err_msg = nullptr;

                if (event.events & StSrtEvent::ST_SRT_ERR) {
                    SRT_SOCKSTATUS status = srt_getsockstate(event.fd);
                    err_msg               = srt_getlasterror_str();
                    err                   = st_srt_socket_error(status);
                    tmss_error("epoll error srt_fd:{} status:{}, err:{}", event.fd, status, err);
                }

                for (auto e = sets.begin(); e != sets.end(); ) {
                    SrtEventCondition* cnd = *e;
                    bool notify = false;

                    // error event notify all the socket
                    if (event.events & StSrtEvent::ST_SRT_ERR) {
                        cnd->err      = err;
                        cnd->err_msg  = err_msg;
                        notify = true;
                    } else {
                        if (event.events & cnd->event) {
                            notify = true;
                        }
                    }

                    if (notify) {
                        sets.erase(e++);
                        st_cond_broadcast(cnd->cond);
                    } else {
                        e++;
                    }
                }

                if (it->second.empty()) {
                    conditions.erase(it);
                }
            }
        }
        st_usleep(10 * 1000);
    }
    return error_success;
}

StSrtSocket::StSrtSocket(SRTSOCKET fd) : fd(fd) {
    rtm = ST_UTIME_NO_TIMEOUT;
    stm = ST_UTIME_NO_TIMEOUT;
}

StSrtSocket::~StSrtSocket() {
    tmss_info("srt socket close fd: {}", fd);
    st_srt_close(fd);
}

void StSrtSocket::set_recv_timeout(utime_t tm) {
    rtm = tm;
}

utime_t StSrtSocket::get_recv_timeout() {
    return rtm;
}

error_t StSrtSocket::read_fully(void* buf, size_t size, ssize_t* nread) {
    size_t n     = 0;
    int    ret   = error_success;
    while (n < size) {
        size_t nr = 0;
        ret       = st_srt_read(fd, reinterpret_cast<char*>(buf)+n, size-n, rtm, nr);
        if (ret == error_success) {
            if (nread) {
                *nread = n;
            }
            return ret;
        }
        n += nr;
    }
    if (nread) {
        *nread = n;
    }
    return error_success;
}

error_t StSrtSocket::read(void* buf, size_t size, size_t& nread) {
    int ret = st_srt_read(fd, buf, size, rtm, nread);
    if (ret != error_success) {
        tmss_error("srt read failed ret:{}", ret);
        return ret;
    }
    return ret;
}

void StSrtSocket::set_send_timeout(utime_t tm) {
    stm = tm;
}

utime_t StSrtSocket::get_send_timeout() {
    return stm;
}

error_t StSrtSocket::write(void* buf, size_t size) {
    size_t write_size = 0;
    return st_srt_write(fd, buf, size, stm, write_size);
}

SRTSOCKET StSrtSocket::get_fd() {
    return fd;
}
