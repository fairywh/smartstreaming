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
#include <memory>
#include "tmss_user_control.hpp"
#include "http_server.hpp"
#include "tmss_channel.hpp"

namespace tmss {
class MediaSource : virtual public IUserHandler, public std::enable_shared_from_this<MediaSource> {
 public:
    MediaSource();
    virtual ~MediaSource();

 public:
    virtual int handle_connect(std::shared_ptr<IClientConn> conn);
    virtual int handle_request(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int handle_cycle(std::shared_ptr<Channel> channel);
    virtual int handle_disconnect(std::shared_ptr<IClientConn> conn);

 private:
    virtual int handle_play(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int handle_publish(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int handle_api(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int create_origin_stream(std::shared_ptr<Channel> channel,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int create_forward(std::shared_ptr<Channel> channel,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int create_origin_file(std::shared_ptr<File>& file,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    // virtual int handle_cycle_segment(std::shared_ptr<Channel> channel);

    virtual int handle_play_stream(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int handle_play_file(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);
    virtual int handle_play_segment(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server);

    std::string create_channel_key(std::shared_ptr<Request> req);

 public:
    virtual int init(int num, char** param);

 private:
    std::vector<std::shared_ptr<IServer>> server_group;
};

int init_default_handler(int num, char** param);
}  // namespace tmss

