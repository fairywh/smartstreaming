/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */
#pragma once

#include <memory>
#include <protocol/client.hpp>
#include <net/tmss_conn.hpp>
#include <coroutine/coroutine.hpp>
#include <cache/tmss_channel.hpp>
#include <tmss_user_control.hpp>
#include <parser.hpp>

namespace tmss {
class IDeMux;
class HttpClient : public IClient {
 public:
    HttpClient();
    virtual ~HttpClient() = default;

 public:
    virtual int cycle() override;
    virtual int request(const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& param,
        std::shared_ptr<IDeMux> demux);
    int read_data(char* buf, int size) override;

    int write_data(const char* buf, int size) override;
};

}  // namespace tmss
