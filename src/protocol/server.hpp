/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/26.
 *        Author:  weideng.
 *
 * =====================================================================================
 */
#pragma once

#include <memory>

#include <net/tmss_conn.hpp>
#include <coroutine/coroutine.hpp>
#include <cache/tmss_channel.hpp>
#include <tmss_user_control.hpp>
#include <parser.hpp>

namespace tmss {
class IServer;
class FileCache;
class IConnManager;
class IConnHandler : public ICoroutineHandler {
 public:
    IConnHandler(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<IServer> server);
    virtual ~IConnHandler() = default;

 public:
    virtual int on_accept(std::shared_ptr<IClientConn> conn) = 0;
    virtual int on_stop() = 0;
    virtual int on_init() = 0;

 public:
    std::shared_ptr<IClientConn> conn;
    std::shared_ptr<IServer> server;
};

class IServer : public ICoroutineHandler {
 public:
    IServer(std::shared_ptr<IServerConn> server_conn,
        std::shared_ptr<ChannelPool> channel_pool,
        std::shared_ptr<FileCache> file_cache);
    virtual ~IServer() = default;

 public:
    virtual int init(const std::string &ip, int port);
    virtual int listen(const std::string &ip, int port) = 0;
    virtual std::shared_ptr<IClientConn> accept() = 0;
    virtual int run();

    virtual int cycle() override;

    virtual std::shared_ptr<IConnHandler> create_conn_handler(std::shared_ptr<IClientConn> conn) = 0;

    std::shared_ptr<ChannelMgr> get_channel_mgr();
    std::shared_ptr<FileCache> get_file_cache();
    std::shared_ptr<Pool<InputHandler>> get_input_pool();
    std::shared_ptr<Pool<OutputHandler>> get_output_pool();
    std::shared_ptr<Pool<FileInputHandler>> get_file_input_pool();
    std::shared_ptr<IConnManager> get_conn_manager();

 protected:
    std::shared_ptr<IServerConn> server_conn;
    //  std::shared_ptr<IConnHandler> handler;

    std::shared_ptr<ChannelMgr> channel_mgr;
    std::shared_ptr<Pool<InputHandler>> input_pool;
    std::shared_ptr<Pool<OutputHandler>> output_pool;
    std::shared_ptr<IConnManager> conn_manager;
    std::shared_ptr<FileCache> file_cache;
    std::shared_ptr<Pool<FileInputHandler>> file_input_pool;

    std::string ip;
    int port;
};

//  class IConnManager;
/*class DefaultConnHandler : public IConnHandler {
 public:
    explicit DefaultConnHandler(std::shared_ptr<IConnManager> conn_manager);
    int on_accept(std::shared_ptr<IClientConn> conn) override;

 public:
    virtual std::shared_ptr<IConnHandler> create_conn_handler(std::shared_ptr<IConn> conn) = 0;

 protected:
    std::shared_ptr<IConnManager> conn_manager;
};//*/

class IConnManager : public ICoroutineHandler {
 public:
    IConnManager();
    virtual ~IConnManager() = default;

 public:
    virtual int cycle() override;
    virtual int register_conn(std::shared_ptr<IConnHandler> conn);
    virtual int remove_conn(std::shared_ptr<IConnHandler> conn);

 private:
    std::set<std::shared_ptr<IConnHandler>> conn_pool;
};
}  // namespace tmss
