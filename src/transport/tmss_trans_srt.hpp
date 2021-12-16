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
#include "st_trans/st_srt.hpp"

namespace tmss {

class SRTServerConn : virtual public IServerConn {
 public:
    SRTServerConn();
    ~SRTServerConn();

    int listen(const std::string &ip, int port);
    std::shared_ptr<IClientConn> accept();
    int close();
    error_t cycle();

 private:
    SRTSOCKET server_socket;
};

class SRTStreamConn : virtual public IClientConn {
 public:
    explicit SRTStreamConn(SRTSOCKET socket);
    ~SRTStreamConn();

    int read(char* buf, int size) override;
    int write(const char* buf, int size) override;
    int connect(Address address) override;
    int close() override;
    error_t cycle();

 public:
    SRTSOCKET client_socket;
    bool    is_connected;
};
}  // namespace tmss
