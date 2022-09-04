/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#pragma once

#include <tmss_cache.hpp>
#include <format/base/context.hpp>
#include <format/base/mux.hpp>
#include <format/base/demux.hpp>
#include <net/tmss_conn.hpp>
#include <protocol/client.hpp>

namespace tmss {
class Channel;
class PacketQueue;

class Session {
};

/*
*   only for long connection
*/
class OutputHandler : public PacketQueue, public ICoroutineHandler {
 public:
    OutputHandler(std::shared_ptr<Pool<OutputHandler>> pool,
        std::shared_ptr<Channel> channel);
    virtual ~OutputHandler();

 private:
    std::weak_ptr<Channel>    channel;
    std::shared_ptr<Session>    session;
    std::shared_ptr<IClientConn> output_conn;
    std::shared_ptr<IClient> client;

    std::shared_ptr<IMux> mux;
    std::shared_ptr<IContext> input_context;
    std::shared_ptr<IContext> output_context;
    bool        is_stop;
    EOutputType output_type;
    EStatusOutput status;
    std::shared_ptr<Pool<OutputHandler>> output_pool;

    Address     forward_address;

    std::string forward_url;
    std::string forward_params;

    int64_t     start_at;
    int64_t     last_send_at;

 public:
    int write_msg(char* buff, int size);
    void init_conn(std::shared_ptr<IClientConn> conn);
    void init_format(std::shared_ptr<IMux> mux);
    void init_play_client(std::shared_ptr<IClient> play_client);
    void set_forward_address(Address& origin_address);
    void set_forward_url(const std::string& origin_url);

    std::shared_ptr<IContext> get_context();
    void set_context(std::shared_ptr<IContext> ctx);

    EOutputType get_type();
    void set_type(EOutputType type);

    int init_output(std::shared_ptr<IContext> input_context);

    int run();
    int set_stop();
    int cycle() override;
    int on_thread_stop() override;
};

}  // namespace tmss

