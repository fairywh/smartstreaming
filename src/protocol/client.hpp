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

#include <net/tmss_conn.hpp>
#include <coroutine/coroutine.hpp>
#include <parser.hpp>
#include <io/io_buffer.hpp>

namespace tmss {
class IDeMux;
/*
*   similar with URLContext
*/
class IClient : public ICoroutineHandler {
 public:
    IClient();
    virtual ~IClient() = default;

 public:
    virtual int init(std::shared_ptr<IClientConn> conn);
    virtual int cycle();
    virtual int request(const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& param,
        std::shared_ptr<IDeMux> demux) = 0;
    /*
    *   return read length
    */
    virtual int read_data(char* buf, int size) = 0;

    virtual int write_data(const char* buf, int size) = 0;

 public:
    std::shared_ptr<IClientConn> conn;

    std::shared_ptr<IOBuffer> io_buffer;
};

}  // namespace tmss
