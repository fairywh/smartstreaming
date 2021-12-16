/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */
#include <tmss_trans_srt.hpp>
#include <defs/err.hpp>
#include <log/log.hpp>
#include <protocol/server.hpp>

namespace tmss {
SRTServerConn::SRTServerConn() {
}

SRTServerConn::~SRTServerConn() {
    close();
}

int SRTServerConn::listen(const std::string &ip, int port) {
    srt_startup();
    server_socket = st_srt_create_fd();
    if (server_socket == SRT_INVALID_SOCK) {
        tmss_error("create sock failed fd:{}", server_socket);
        return error_srt_socket_create;
    }
    int ret = st_srt_listen(server_socket, ip, port,
        1024);
    if (ret != error_success) {
        tmss_error("listen sock failed fd:{}, port:{}", server_socket, port);
        return ret;
    }
    return 0;
}

std::shared_ptr<IClientConn> SRTServerConn::accept() {
    sockaddr_in addr {};
    int         addr_len = sizeof(addr);

    std::shared_ptr<IClientConn> conn;
    SRTSOCKET cfd = st_srt_accept(server_socket, (struct sockaddr*)&addr, &addr_len,
        ST_UTIME_NO_TIMEOUT);
    if (cfd == SRT_INVALID_SOCK) {
        tmss_info("timeout accept retry");
        return conn;
    } else {
        tmss_info("srt accept new connection");
    }

    conn = std::make_shared<SRTStreamConn>(cfd);
    return conn;
}

int SRTServerConn::close() {
    return st_srt_close(server_socket);
}

error_t SRTServerConn::cycle() {
    return error_success;
}

SRTStreamConn::SRTStreamConn(SRTSOCKET client_fd) {
    this->client_socket = client_fd;
    if (client_fd) {
        // not null, connected
        is_connected = true;
    } else {
        is_connected = false;
    }
    tmss_info("create new conn, id={}", get_id());
}

SRTStreamConn::~SRTStreamConn() {
    tmss_info("~conn, id={}", get_id());
    close();
}

// connect to address
int SRTStreamConn::connect(Address address) {
    int ret = error_success;
    client_socket = srt_create_socket();
    if (client_socket == SRT_ERROR) {
        tmss_error("srt create error, id{}, ret={}", get_id(), ret);
        return -1;
    } else {
    }
    ret = st_srt_connect(client_socket, address.get_ip(), address.get_port(),
        (connect_timeout_ms < 0) ? -1 : (utime_t)connect_timeout_ms * 1000);
    if (ret == SRT_ERROR) {
        tmss_error("srt connect error, id{}, ret={}", get_id(), ret);
        return ret;
    }
    is_connected = true;
    return ret;
}

int SRTStreamConn::write(const char * buf, int size) {
    if (!is_connected) {
        tmss_info("not connect, id={}", get_id());
        return error_srt_socket_already_closed;
    }

    size_t write_size = 0;
    int ret = st_srt_write(client_socket, buf, size,
        (send_timeout_ms < 0) ? -1 : (utime_t)send_timeout_ms * 1000, write_size);
    if (ret == SRT_ERROR) {
        tmss_error("srt write error, id{}, ret={}", get_id(), ret);
        return ret;
    }

    if (write_size != size) {
        tmss_error("srt write not complete, id{}, {}/{}", get_id(), write_size, size);
    }

    tmss_info("srt write complete, id{}, {}/{}", get_id(), write_size, size);
    return write_size;
}

// receive data
int SRTStreamConn::read(char* buf, int size) {
    if (!is_connected) {
        tmss_info("not connect, id={}", get_id());
        return error_srt_socket_already_closed;
    }

    size_t read_size = 0;
    int ret = st_srt_read(client_socket, buf, size,
        (recv_timeout_ms < 0) ? -1 : (utime_t)recv_timeout_ms * 1000, read_size);
    if (ret == SRT_ERROR) {
        tmss_error("srt read error, id{}, ret={}", get_id(), ret);
        return ret;
    }

    if (read_size != size) {
        tmss_info("srt read not complete, id{}, {}/{}", get_id(), read_size, size);
    }
    return read_size;
}

int SRTStreamConn::close() {
    if (!is_connected) {
        tmss_info("already close, id={}", get_id());
        return error_success;
    }

    if (handler.lock()) {
        handler.lock()->stop();
        tmss_info("stop_handler,fd={}, id={}", client_socket, get_id());
    }
    int ret = srt_close(client_socket);
    if (ret == SRT_ERROR) {
        tmss_error("conn close error, id={}, ret={}", get_id(), ret);
        return -1;
    }

    tmss_info("conn close, id={}", get_id());

    is_connected = false;

    return 0;
}

error_t SRTStreamConn::cycle() {
    return error_success;
}

}  // namespace tmss
