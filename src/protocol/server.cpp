/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/26.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#include <protocol/server.hpp>

#include <utility>

#include <defs/err.hpp>
#include <log/log.hpp>

namespace tmss {
const int max_pool_size = 10000;
IConnHandler::IConnHandler(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<IServer> server) : ICoroutineHandler("conn_handler") {
    this->conn = conn;
    this->server = server;
}

/*DefaultConnHandler::DefaultConnHandler(std::shared_ptr<IConnManager> conn_manager) :
    conn_manager(std::move(conn_manager)) {
}

int DefaultConnHandler::on_accept(std::shared_ptr<IConn> conn) {
    auto handler = create_conn_handler(conn);
    int ret = handler->start();
    if (ret != error_success) {
        tmss_error("accept failed ret:%d", ret);
        return ret;
    }

    conn_manager->register_conn(conn);
    return ret;
}//*/

IServer::IServer(std::shared_ptr<IServerConn> server_conn,
        std::shared_ptr<ChannelPool> channel_pool,
        std::shared_ptr<FileCache> file_cache) : ICoroutineHandler("server_base") {
    this->server_conn = server_conn;
    channel_mgr = std::make_shared<ChannelMgr>(channel_pool);
    conn_manager = std::make_shared<IConnManager>();
    input_pool = std::make_shared<Pool<InputHandler>>(max_pool_size);
    output_pool = std::make_shared<Pool<OutputHandler>>(max_pool_size);
    file_input_pool = std::make_shared<Pool<FileInputHandler>>(max_pool_size);
    this->file_cache = file_cache;
}

int IServer::init(const std::string &ip, int port) {
    int ret = error_success;
    this->ip = ip;
    this->port = port;

    return ret;
}

int IServer::run() {
    int ret = this->listen(ip, port);
    if (ret != error_success) {
        tmss_error("run failed, listen error,ret={}", ret);
        return ret;
    }
    // start co_thread
    ret = start();
    if (ret != error_success) {
        tmss_error("run failed, start co_thread error,ret={}", ret);
        return ret;
    }

    ret = server_conn->run();
    if (ret != error_success) {
        tmss_error("run failed, server_conn start error,ret={}", ret);
        return ret;
    }

    return ret;
}

int IServer::cycle() {
    while (true) {
        // accept
        std::shared_ptr<IClientConn> conn = this->accept();
        if (!conn) {
            tmss_error("conn is null");
            continue;
        }
        if (conn) {
            conn->run();
        }

        std::shared_ptr<IConnHandler> handler = this->create_conn_handler(conn);
        if (!handler->server) {
            tmss_error("server is null");
        }

        handler->on_init();

        int ret = handler->on_accept(conn);
        if (ret != error_success) {
            return ret;
        }
        tmss_info("output_conn ref_count={}", conn.use_count());
        conn_manager->register_conn(handler);
        // tmss_info("output_conn ref_count={}", conn.use_count());
    }
    return error_success;
}

std::shared_ptr<ChannelMgr> IServer::get_channel_mgr() {
    return channel_mgr;
}
std::shared_ptr<FileCache> IServer::get_file_cache() {
    return file_cache;
}

std::shared_ptr<Pool<InputHandler> > IServer::get_input_pool() {
    return input_pool;
}

std::shared_ptr<Pool<OutputHandler> > IServer::get_output_pool() {
    return output_pool;
}

std::shared_ptr<Pool<FileInputHandler> > IServer::get_file_input_pool() {
    return file_input_pool;
}

std::shared_ptr<IConnManager> IServer::get_conn_manager() {
    return conn_manager;
}

IConnManager::IConnManager() : ICoroutineHandler("conn_manager") {
}

/*IConnManager::~IConnManager() {
    conn_pool.clear();
}//*/

int IConnManager::cycle() {
    //  check expire
    int ret = error_success;
    return ret;
}

int IConnManager::register_conn(std::shared_ptr<IConnHandler> conn) {
    int ret = error_success;
    conn_pool.insert(conn);

    return ret;
}

int IConnManager::remove_conn(std::shared_ptr<IConnHandler> conn) {
    int ret = error_success;
    conn_pool.erase(conn);

    return ret;
}



}   // namespace tmss
