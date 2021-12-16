/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#pragma once

#include "tmss_cache.hpp"
#include <format/context.hpp>
#include <format/mux.hpp>
#include <format/demux.hpp>
#include <net/tmss_conn.hpp>
#include <net/parser.hpp>
#include <protocol/client.hpp>


namespace tmss {
class Channel;
class PacketQueue;
class IClient;
class InputHandler : public PacketQueue, public ICoroutineHandler {
 private:
    std::weak_ptr<Channel>    channel;
    Address     origin_address;

    Request origin_request;
    EInputType         input_type;

    // to do, maybe there are multiple transport, such as rtsp over rtp and rtcp
    std::shared_ptr<IClientConn> input_conn;
    std::shared_ptr<IClient> client;

    std::shared_ptr<IDeMux> demux;

    std::shared_ptr<IContext> context;

    EStatusSource status;
    bool        is_stop;

 public:
    InputHandler(std::shared_ptr<Pool<InputHandler>> pool,
        std::shared_ptr<Channel> channel);
    ~InputHandler();

    int set_connection(std::shared_ptr<IClientConn> conn);
    bool can_use() override;
    int fetch_stream(char* buff, int wanted_size);
    void init_conn(std::shared_ptr<IClientConn> conn);
    void init_format(std::shared_ptr<IDeMux> demux);
    void init_origin_client(std::shared_ptr<IClient> origin_client);
    void set_origin_address(Address& origin_address);
    void set_origin_info(Address& origin_address,
        const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& ext,
        const std::string& params);
    std::shared_ptr<IContext> get_context();
    void set_context(std::shared_ptr<IContext> ctx);
    EInputType get_type();
    void set_type(EInputType type);

    int init_input();

    int run();
    int set_stop();

    int cycle() override;
    int on_thread_stop() override;
    int probe();
    int cycle_interleave();

 private:
    int origin_start();
    std::shared_ptr<Pool<InputHandler>> input_pool;

    int origin_try_count;
};

}  // namespace tmss

