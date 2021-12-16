/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */
#include <sstream>
#include <tmss_conn.hpp>
#include <log/log.hpp>
namespace tmss {
int conn_id = 0;
const int default_connect_timeout_ms = 3 * 1000;
const int default_send_timeout_ms = 3 * 1000;
const int default_recv_timeout_ms = 3 * 1000;
IConn::IConn() {
    id = conn_id++;
}

IServerConn::IServerConn() : ICoroutineHandler("server_conn") {
}

IServerConn::~IServerConn() {
}

int IServerConn::run() {
    return start();
}
IClientConn::IClientConn() : ICoroutineHandler("client_conn") {
    stop = false;
    connect_timeout_ms = default_connect_timeout_ms;
    send_timeout_ms = default_send_timeout_ms;
    recv_timeout_ms = default_recv_timeout_ms;
}

IClientConn::~IClientConn() {
    // close
}

int IClientConn::run() {
    return start();
}

void IClientConn::set_stop() {
    tmss_info("client_conn set_stop, id={}", get_id());
    stop = true;
}
bool IClientConn::is_stop() {
    return stop;
}

void IClientConn::set_handler(std::shared_ptr<IConnHandler> conn_handler) {
    handler = conn_handler;
}

void IClientConn::set_connect_timeout(int32_t timeout_ms) {
    connect_timeout_ms = timeout_ms;
}
void IClientConn::set_send_timeout(int32_t timeout_ms) {
    send_timeout_ms = timeout_ms;
}
void IClientConn::set_recv_timeout(int32_t timeout_ms) {
    recv_timeout_ms = timeout_ms;
}

Address::Address() {
}

Address::Address(const std::string& ip, int port) :
    remote_ip(ip), remote_port(port) {
}

Address::~Address() {
}

std::string Address::str() {
    std::ostringstream result;
    result << remote_ip << ":" << remote_port;
    return result.str();
}

}  // namespace tmss

