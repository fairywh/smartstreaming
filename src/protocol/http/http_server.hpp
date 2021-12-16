/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/26.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */
#pragma once

#include <map>
#include <memory>
#include <list>
#include <string>
#include <utility>

#include <http_parser.hpp>
#include <server.hpp>
#include <demux.hpp>
#include <io_buffer.hpp>
#include <transport/tmss_trans_tcp.hpp>

/**
 * 参考srs内的http mux逻辑
 */

namespace tmss {
class HttpServer;
class IUserHandler;
class HttpMuxEntry {
 public:
    HttpMuxEntry();
    virtual ~HttpMuxEntry() = default;

 public:
    bool can_server(std::shared_ptr<HttpRequest> req);
    std::shared_ptr<IUserHandler> get_user_handler();

 private:
    bool explicit_match;
    bool enabled;

 public:
    std::shared_ptr<IUserHandler> handler;
    std::string pattern;
};

enum HttpMuxLevel {
    HIGH   = 0,
    NORMAL = 1,
    LOW    = 100,
};

class HttpLevelMux {
 public:
    friend class HttpMux;

 private:
    int level;
    //  std::list<HttpMuxEntry> muxer;    // to do
    HttpMuxEntry muxer;
};

class HttpMux {
 public:
    HttpMux();
    int register_handler(std::string pattern, int level, std::shared_ptr<IUserHandler> h);
    int delete_handler(std::string& pattern, int level = -1);
    int get_handler(std::string pattern, std::shared_ptr<IUserHandler>& handler);

 public:
    int serve_http(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<HttpRequest> req,
        std::shared_ptr<IServer> server);

 public:
    std::map<int, std::shared_ptr<HttpLevelMux>> level_handlers;
    int last_level;

 public:
    static std::shared_ptr<HttpMux> get_instance();
};

class HttpConnHandler : public IConnHandler {
 public:
    HttpConnHandler(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<HttpServer> server);
    int cycle() override;
    int on_thread_stop() override;
    int on_accept(std::shared_ptr<IClientConn> conn) override;
    int on_stop() { return error_success;  /*return manager->remove_conn(this);*/ }
    int on_init();
};

/**
 * on_accept -> http_mux -> server_http
 */
class HttpServer : public IServer {
 public:
    HttpServer(std::shared_ptr<IServerConn> server_conn,
        std::shared_ptr<ChannelPool> channel_pool,
        std::shared_ptr<FileCache> file_cache);
    ~HttpServer();

 public:
    int listen(const std::string &ip, int port) override;
    std::shared_ptr<IConnHandler> create_conn_handler(std::shared_ptr<IClientConn> conn) override;
    std::shared_ptr<IClientConn> accept() override;
};

}  // namespace tmss
