/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#include <protocol/client.hpp>

#include <utility>

#include <defs/err.hpp>
#include <log/log.hpp>

namespace tmss {
IClient::IClient() : ICoroutineHandler("client") {
}

int IClient::init(std::shared_ptr<IClientConn> conn) {
    this->conn = conn;
    int ret = error_success;
    int in_buf_size = 1024 * 16;
    char* in_buf = new char[in_buf_size];
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(in_buf, in_buf_size);
    io_buffer = std::make_shared<IOBuffer>(this->conn, buffer);

    return ret;
}

int IClient::cycle() {
    int ret = error_success;
    return ret;
}

}   // namespace tmss
