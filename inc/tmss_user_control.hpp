/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu.
 *
 * =====================================================================================
 */

#pragma once
#include <string>
#include <memory>
#include <parser.hpp>

namespace tmss {
class IClientConn;
class IServer;
class Channel;
class IUserHandler {
 public:
    IUserHandler() = default;
    virtual ~IUserHandler() = default;
 public:
    virtual int handle_connect(std::shared_ptr<IClientConn> conn) = 0;
    virtual int handle_request(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) = 0;
    virtual int handle_cycle(std::shared_ptr<Channel> channel) = 0;
    virtual int handle_disconnect(std::shared_ptr<IClientConn> conn) = 0;
};
}  // namespace tmss