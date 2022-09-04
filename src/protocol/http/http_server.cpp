/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/26.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#include <http_server.hpp>
#include <defs/err.hpp>
#include <http/http_parser.hpp>
#include <log/log.hpp>

namespace tmss {
HttpMuxEntry::HttpMuxEntry() {
}

bool HttpMuxEntry::can_server(std::shared_ptr<HttpRequest> req) {
    return true;
}

std::shared_ptr<IUserHandler> HttpMuxEntry::get_user_handler() {
    return handler;
}

HttpMux::HttpMux() {
    last_level = 0;
}

int HttpMux::register_handler(std::string pattern,
        int level,
        std::shared_ptr<IUserHandler> user_handler) {
    std::shared_ptr<HttpLevelMux> http_level_mux = std::make_shared<HttpLevelMux>();
    http_level_mux->muxer.handler = user_handler;
    http_level_mux->muxer.pattern = pattern;
    level_handlers.insert(std::make_pair(last_level++, http_level_mux));

    tmss_error("register handler, pattern={}", pattern);
    return error_success;
}
int HttpMux::delete_handler(std::string& pattern, int level) {
    auto http_level_mux = level_handlers.find(level);
    if (http_level_mux != level_handlers.end()) {
        level_handlers.erase(http_level_mux);
    }

    tmss_error("delete handler, pattern={}", pattern);

    return error_success;
}

int HttpMux::get_handler(std::string pattern, std::shared_ptr<IUserHandler>& handler) {
    // level_handlers
    int ret = error_success;
    if (level_handlers.empty()) {
        //  error
        tmss_error("no handler, pattern={}", pattern);
        ret = error_system_handler_not_found;
        return ret;
    }
    for (auto& http_mux : level_handlers) {
        HttpMuxEntry mux_entry = http_mux.second->muxer;  // to do
        tmss_info("check handler, pattern={}", mux_entry.pattern);
        if (mux_entry.pattern == pattern) {     // to do
            handler = mux_entry.get_user_handler();
            tmss_info("get handler, pattern={}", mux_entry.pattern);
        }
    }
    return ret;
}

int HttpMux::serve_http(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<HttpRequest> req,
        std::shared_ptr<IServer> server) {
    // to do , maybe there are many request during connectino
    int ret = error_success;
    std::shared_ptr<IUserHandler> user_handler;
    ret = get_handler("/", user_handler);
    if (ret != error_success) {
        tmss_error("get handler failed, ret={}", ret);
        return ret;
    }
    if (user_handler) {
        tmss_info("find handler");
        // user logic, like, 1.create channel/output/input, 2.start channel
        if (!server) {
            ret = error_system_undefined;
            tmss_error("serve_http failed, server null");
            return ret;
        }
        if (user_handler->handle_request(conn, req, server) != 0) {
        } else {
        }
    } else {
        // error
        ret = error_system_handler_not_found;
        tmss_error("serve_http failed, ret={}", ret);
        return ret;
    }

    // read connection, if error, return and stop thread
    conn->set_recv_timeout(-1);     // no timeout
    while (true) {
        if (conn->is_stop()) {
            tmss_info("conn stop");
            break;
        }
        std::shared_ptr<HttpRequest> new_req = std::make_shared<HttpRequest>();
        HttpParser parser;
        // to do
        ret = parser.parse_request(conn, new_req);
        if (ret != error_success) {
            tmss_error("http parse failed, ret={}", ret);
            break;
        }
    }
    if (!conn->is_stop()) {
        tmss_info("conn stop by http_server");
        conn->set_stop();
    }
    return ret;
}

std::shared_ptr<HttpMux> HttpMux::get_instance() {
    thread_local std::shared_ptr<HttpMux> ins = nullptr;
    if (!ins) {
        ins = std::make_shared<HttpMux>();
    }
    return ins;
}

HttpConnHandler::HttpConnHandler(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<HttpServer> server) : IConnHandler(conn, server) {
}

int HttpConnHandler::cycle() {
    HttpParser parser;
    std::shared_ptr<HttpRequest> req;

    int ret = parser.parse_request(conn, req);
    if (ret != error_success) {
        tmss_error("http parse failed, ret={}", ret);
        return ret;
    }
    tmss_info("req=vhost={},path={},streamid={},ext={}",
        req->vhost,
        req->path,
        req->name,
        req->ext);
    // set format
    /*if (req->ext == "flv") {     // test
        req->format = "flv";
        // req->format = "RAW";
    } else if (req->ext == "ts") {
        req->format = "mpegts";
    } else {
        req->format = "RAW";
    }   //*/

    // tmss_info("output_conn ref_count={}", conn.use_count());

    return HttpMux::get_instance()->serve_http(conn, req, server);
}

int HttpConnHandler::on_thread_stop() {
    if (!server) {
        return error_success;
    }
    tmss_info("http conn handler stop, output_conn ref_count={}", conn.use_count());
    int ret = server->get_conn_manager()->remove_conn(
        std::dynamic_pointer_cast<HttpConnHandler>(shared_from_this()));
    // tmss_info("output_conn ref_count={}", conn.use_count());
    return ret;
}

int HttpConnHandler::on_accept(std::shared_ptr<IClientConn> conn) {
    // start thread
    int ret = this->start();
    if (ret != error_success) {
        //  tmss_error("accept failed ret:%d", ret);
        return ret;
    }
    return ret;
}

int HttpConnHandler::on_init() {
    conn->set_handler(std::dynamic_pointer_cast<IConnHandler>(shared_from_this()));
    return error_success;
}

HttpServer::HttpServer(std::shared_ptr<IServerConn> server_conn,
        std::shared_ptr<ChannelPool> channel_pool,
        std::shared_ptr<FileCache> file_cache) :
    IServer(server_conn, channel_pool, file_cache) {
}

HttpServer::~HttpServer() {
}

int HttpServer::listen(const std::string &ip, int port) {
    return server_conn->listen(ip, port);
}

std::shared_ptr<IConnHandler> HttpServer::create_conn_handler(std::shared_ptr<IClientConn> conn) {
    return std::make_shared<HttpConnHandler>(conn,
        std::dynamic_pointer_cast<HttpServer>(shared_from_this()));
}

std::shared_ptr<IClientConn> HttpServer::accept() {
    std::shared_ptr<IClientConn> conn = server_conn->accept();
    if (!conn) {
        tmss_error("conn is null");
    }
    tmss_info("output_conn ref_count={}", conn.use_count());
    return conn;
}
}  // namespace tmss
