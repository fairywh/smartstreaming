/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#pragma once

#include <net/tmss_conn.hpp>
#include "st_trans/st_tcp.hpp"

namespace tmss {
class TcpServerConn : virtual public IServerConn {
 public:
    TcpServerConn();
    ~TcpServerConn();

 public:
    int listen(const std::string &ip, int port);
    std::shared_ptr<IClientConn> accept();
    int close();
    error_t cycle();

 private:
    st_netfd_t server_fd;
};

class TcpStreamConn : public IClientConn {
 public:
    explicit TcpStreamConn(st_netfd_t client_fd);
    virtual ~TcpStreamConn();

 public:
    int read(char* buf, int size) override;
    int read_fully(char* buf, int size) override;
    int write(const char* buf, int size) override;
    int writev(const iovec *iov, int iov_size) override;
    int connect(Address address) override;
    int close() override;
    error_t cycle();

    virtual int64_t get_recv_bytes() override;
    virtual int64_t get_send_bytes() override;

 private:
    st_netfd_t client_fd;
    bool    is_connected;

    int64_t recv_bytes;
    int64_t send_bytes;
};

}  // namespace tmss

