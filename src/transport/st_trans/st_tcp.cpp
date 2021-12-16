/* Copyright [2019] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/14.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */

#include "st_tcp.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include <string>

#include "log/log.hpp"


st_netfd_t st_open_socket_only(int fd) {
    return st_netfd_open_socket_only(fd);
}

void st_free_socket_only(st_netfd_t fd) {
    return st_netfd_free(fd);
}

std::string dns_resolve(const std::string& host) {
    if (inet_addr(host.c_str()) != INADDR_NONE) {
        return host;
    }

    hostent* answer = gethostbyname(host.c_str());
    if (answer == nullptr) {
        return "";
    }

    char ipv4[16];
    memset(ipv4, 0, sizeof(ipv4));
    if (0 < answer->h_length) {
        inet_ntop(AF_INET, answer->h_addr_list[0], ipv4, sizeof(ipv4));
    }

    return ipv4;
}

int st_tcp_fd(st_netfd_t fd) {
    return st_netfd_fileno(fd);
}

error_t st_tcp_create_fd(st_netfd_t& fd) {
    fd = nullptr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return error_socket_create;
    }
    fd = st_netfd_open_socket(sock);
    if (fd == nullptr) {
        ::close(sock);
        return error_socket_open;
    }
    return error_success;
}

st_netfd_t st_tcp_open_fd(int fd) {
    return st_netfd_open_socket(fd);
}

error_t st_tcp_close(st_netfd_t& fd) {
    int ret = st_netfd_close(fd);
    if (ret != error_success) {
        tmss_error("close error, errno={},ret={}", errno, ret);
        return ret;
    }
    fd = nullptr;
    return error_success;
}

error_t st_tcp_listen(const std::string& server, int port,
                      int back_log, st_netfd_t& fd, bool reuse_port) {
    error_t ret = st_tcp_create_fd(fd);

    if (ret != error_success) {
        return ret;
    }

    addrinfo  hints{};
    char      sport[8];
    addrinfo* r       = nullptr;
    int       fileno  = st_netfd_fileno(fd);

    snprintf(sport, sizeof(sport), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_NUMERICHOST;

    if ((ret = st_tcp_fd_reuseaddr(fileno)) != error_success) {
        goto failed;
    }

    if (reuse_port) {
        if ((ret = st_tcp_fd_reuseport(fileno)) != error_success) {
            goto failed;
        }
    }

    if (getaddrinfo(server.c_str(), sport, (const addrinfo*)&hints, &r)) {
        ret = error_socket_bind;
        goto failed;
    }

    if (::bind(fileno, r->ai_addr, r->ai_addrlen) == -1) {
        ret = error_socket_bind;
        goto failed;
    }

    if (::listen(fileno, back_log) == -1) {
        ret = error_socket_listen;
        goto failed;
    }

    free(r);
    return ret;

failed:
    st_tcp_close(fd);
    free(r);
    return ret;
}

error_t st_tcp_connect(const std::string& server, int port, utime_t tm, st_netfd_t* fd) {
    error_t ret     = error_success;
    *fd             = nullptr;
    st_netfd_t stfd = nullptr;
    sockaddr_in addr{};

    st_tcp_create_fd(stfd);
    if (stfd == nullptr) {
        ret = error_socket_open;
        return ret;
    }

    // connect to server.
    std::string ip = dns_resolve(server);
    if (ip.empty()) {
        ret = error_system_ip_invalid;
        goto failed;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (st_connect(stfd, (const struct sockaddr*) &addr, sizeof(addr), tm) == -1) {
        ret = error_socket_connect;
        goto failed;
    }

    *fd = stfd;
    return ret;

    failed:
    if (stfd) {
        st_tcp_close(stfd);
    }
    return ret;
}

st_netfd_t st_tcp_accept(st_netfd_t stfd, struct sockaddr *addr, int *addrlen, utime_t timeout) {
    return st_accept(stfd, addr, addrlen, timeout);
}

error_t st_tcp_read(st_netfd_t stfd, void *buf, size_t nbyte, utime_t timeout) {
    return st_read(stfd, buf, nbyte, timeout);
}

error_t st_tcp_readv(st_netfd_t stfd, const struct iovec *iov, int iov_size,
        st_utime_t timeout) {
    return st_readv(stfd, iov, iov_size, timeout);
}

error_t st_tcp_write(st_netfd_t stfd,  const void *buf, size_t nbyte, utime_t timeout) {
    return st_write(stfd, buf, nbyte, timeout);
}

error_t st_tcp_writev(st_netfd_t stfd, const iovec *iov,
                      int iov_size, utime_t timeout) {
    ssize_t n = st_writev(stfd, iov, iov_size, timeout);
    return n;
}

error_t st_tcp_fd_reuseaddr(int fd, int enable) {
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        return error_socket_set_resuse;
    }

    return error_success;
}

error_t st_tcp_fd_reuseport(int fd) {
    int v = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &v, sizeof(int)) == -1) {
        return error_socket_set_resuse;
    }
    return error_success;
}

StTcpSocket::StTcpSocket(st_netfd_t stfd) {
    this->stfd = stfd;
    rtm = stm = ST_UTIME_NO_TIMEOUT;
    rbytes = sbytes = 0;
}

StTcpSocket::~StTcpSocket() {
    //  tmss_info("tcp socket close, fd:{}", st_tcp_fd(stfd));
    st_tcp_close(stfd);
}

// reader
void StTcpSocket::set_recv_timeout(utime_t tm) {
    rtm = tm;
}

utime_t StTcpSocket::get_recv_timeout() {
    return rtm;
}

error_t StTcpSocket::read_fully(void* buf, size_t size, ssize_t* nread) {
    error_t ret = error_success;
    ssize_t n   = st_read_fully(stfd, buf, size, rtm);

    if (nread) {
        *nread = n;
    }

    if (n != (ssize_t)size) {
        // @see https://github.com/ossrs/srs/issues/200
        if (n < 0 && errno == ETIME) {
            return error_socket_timeout;
        }

        if (n >= 0) {
            errno = ECONNRESET;
        }

        return error_socket_read;
    }

    rbytes += n;
    return ret;
}

error_t StTcpSocket::read(void* buf, size_t size, size_t& nread) {
    error_t nb_read = st_read(stfd, buf, size, rtm);
    if (nb_read <= 0) {
        // @see https://github.com/ossrs/srs/issues/200
        if (nb_read < 0 && errno == ETIME) {
            return error_socket_timeout;
        }

        if (nb_read == 0) {
            errno = ECONNRESET;
            return error_socket_already_closed;
        }

        if (errno == EINTR) {
            return error_socket_read;
        }
        return error_socket_read;
    }

    rbytes += nb_read;
    nread = nb_read;
    return error_success;
}

void StTcpSocket::set_send_timeout(utime_t tm) {
    this->stm = tm;
}

utime_t StTcpSocket::get_send_timeout() {
    return stm;
}

error_t StTcpSocket::write(void* buf, size_t size) {
    error_t ret = error_success;

    ssize_t nb_write = st_write(stfd, buf, size, stm);

    // On success a non-negative integer equal to nbyte is returned.
    // Otherwise, a value of -1 is returned and errno is set to indicate the error.
    if (nb_write <= 0) {
        // @see https://github.com/ossrs/srs/issues/200
        if (nb_write < 0 && errno == ETIME) {
            return error_socket_timeout;
        }
        return error_socket_write;
    }

    sbytes += nb_write;

    return ret;
}
