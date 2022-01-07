/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng.
 *
 * =====================================================================================
 */
#include <tmss_trans_tcp.hpp>
#include <defs/err.hpp>
#include <log/log.hpp>
#include <protocol/server.hpp>

namespace tmss {
TcpServerConn::TcpServerConn() {
}

TcpServerConn::~TcpServerConn() {
    close();
}

int TcpServerConn::listen(const std::string &ip, int port) {
    // bind
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return error_socket_create;
    }
    int ret = st_tcp_fd_reuseaddr(sock, 1);
    if (ret != error_success) {
        return ret;
    }
    server_fd = st_tcp_open_fd(sock);
    // listen
    if ((ret = st_tcp_listen(ip, port,
            1024, server_fd, ST_UTIME_NO_TIMEOUT)) != error_success) {
        tmss_error("tcp listen error, {}:{}", ip.c_str(), port);
        return ret;
    } else {
        tmss_info("tcp listen success, {}:{}", ip.c_str(), port);
    }
    return ret;
}

std::shared_ptr<IClientConn> TcpServerConn::accept() {
    sockaddr_in addr {};
    int         addr_len = sizeof(addr);
    // co accept
    std::shared_ptr<IClientConn> conn;
    st_netfd_t  clfd     = nullptr;
    if ((clfd = st_tcp_accept(server_fd, (struct sockaddr*) &addr,
                              &addr_len, ST_UTIME_NO_TIMEOUT)) == nullptr) {
        tmss_error("tcp accept failed");
        return conn;
    } else {
        tmss_info("tcp accept new connection");
    }
    conn = std::make_shared<TcpStreamConn>(clfd);
    // int fd = st_tcp_fd(clfd);
    // st_free_socket_only(clfd);

    return conn;
}

int TcpServerConn::close() {
    return st_tcp_close(server_fd);
}

error_t TcpServerConn::cycle() {
    return error_success;
}

TcpStreamConn::TcpStreamConn(st_netfd_t client_fd) {
    this->client_fd = client_fd;
    if (client_fd) {
        // not null, connected
        is_connected = true;
    } else {
        is_connected = false;
    }
    tmss_info("create new conn, id={}", get_id());
    recv_bytes = send_bytes = 0;
}

TcpStreamConn::~TcpStreamConn() {
    tmss_info("~conn, id={}", get_id());
    close();
}

int TcpStreamConn::read(char* buf, int size) {
    if (!is_connected) {
        tmss_info("not connect, id={}", get_id());
        return error_socket_already_closed;
    }
    // to do, buffer
    int ret = st_tcp_read(client_fd, buf, size,
        (recv_timeout_ms < 0) ? -1 : (utime_t)recv_timeout_ms * 1000);
    if (ret < 0) {
        tmss_error("tcp read error, id{}, ret={}", get_id(), ret);
        return ret;
    }
    if (ret != size) {
        tmss_info("tcp read not complete, id{}, {}/{}", get_id(), ret, size);
    }
    recv_bytes += ret;
    tmss_info("tcp read, {}/{}", ret, size);
    return ret;
}

int TcpStreamConn::read_fully(char* buf, int size) {
    int ret = error_success;

    return ret;
}

int TcpStreamConn::write(const char* buf, int size) {
    if (!is_connected) {
        tmss_info("not connect, id={}", get_id());
        return error_socket_already_closed;
    }
    int ret = st_tcp_write(client_fd, buf, size,
        (send_timeout_ms < 0) ? -1 : (utime_t)send_timeout_ms * 1000);
    if (ret < 0) {
        tmss_error("tcp write error, id{}, ret={}", get_id(), ret);
        return ret;
    }
    if (ret != size) {
        tmss_error("tcp write not complete, id{}, {}/{}", get_id(), ret, size);
    }

    tmss_info("tcp write complete, id{}, {}/{}", get_id(), ret, size);

    send_bytes += ret;
    return ret;
}

int TcpStreamConn::writev(const iovec *iov, int iov_size) {
    if (!is_connected) {
        tmss_info("not connect, id={}", get_id());
        return error_socket_already_closed;
    }
    int ret = st_tcp_writev(client_fd, iov, iov_size,
        (send_timeout_ms < 0) ? -1 : static_cast<utime_t>(send_timeout_ms * 1000));
    if (ret < 0) {
        tmss_error("tcp write error, id{}, ret={}", get_id(), ret);
        return ret;
    }

    tmss_info("tcp write complete, id{}, {}", get_id(), ret);

    send_bytes += ret;
    return ret;
}

int TcpStreamConn::connect(Address address) {
    int ret = st_tcp_connect(address.get_ip(), address.get_port(),
        (connect_timeout_ms < 0) ? -1 : (utime_t)connect_timeout_ms * 1000,
        &client_fd);
    if (ret != 0) {
        tmss_error("tcp connect error, id{}, ret={}", get_id(), ret);
        return ret;
    }
    is_connected = true;
    return ret;
}
int TcpStreamConn::close() {
    if (!is_connected) {
        tmss_info("already close, id={}", get_id());
        return error_success;
    }
    int fd = st_tcp_fd(client_fd);
    tmss_info("fd={}, id={}", fd, get_id());

    if (handler.lock()) {
        handler.lock()->stop();
        tmss_info("stop_handler,fd={}, id={}", fd, get_id());
    }
    int ret = st_tcp_close(client_fd);
    if (ret != 0) {
        tmss_error("conn close error, id={}, ret={}", get_id(), ret);
        return ret;
    }
    tmss_info("conn close, id={}", get_id());

    is_connected = false;

    return ret;
}

error_t TcpStreamConn::cycle() {
    return error_success;
}

int64_t TcpStreamConn::get_recv_bytes() {
    return recv_bytes;
}

int64_t TcpStreamConn::get_send_bytes() {
    return send_bytes;
}

}  // namespace tmss
