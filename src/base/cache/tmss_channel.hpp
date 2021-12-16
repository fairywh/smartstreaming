/*
 * TMSS
 * Copyright (c) 2021 rainwu
 */

#pragma once
#include <vector>
#include <set>
#include <string>
#include <map>
#include <tmss_input.hpp>
#include <tmss_output.hpp>
#include <tmss_cache.hpp>
#include "tmss_segment.hpp"

namespace tmss {
enum EStatusChannel {
    EChannelInit = 0,
    EChannelStart = 1,
    EChannelKill = 2
};

class IUserHandler;
class ChannelPool;

class ChannelMgr {
 public:
    explicit ChannelMgr(std::shared_ptr<ChannelPool> pool);
    ~ChannelMgr();
    int fetch_or_create_channel(const std::string& hash_key, std::shared_ptr<Channel>& new_channel);

 private:
    std::shared_ptr<ChannelPool> pool;
    //  static std::shared_ptr<ChannelMgr> get_instance();
};

class Channel : public ICoroutineHandler {
 public:
    Channel();
    explicit Channel(const std::string& key);
    ~Channel();

 protected:
    std::string                    key;            // hash_key, this is unique
    std::vector<std::shared_ptr<PacketQueue> > input_queue;     // get packet from other input or channel
    std::vector<std::shared_ptr<PacketQueue> > output_queue;    // dispatch packet to other output or channel
    std::shared_ptr<IUserHandler>           user_ctrl;
    std::shared_ptr<IContext>          context;
    SortedCache<IPacket>          cache;
    std::string             stream_id;          // file name in url

    int64_t                 idle_at;            // no input or output
    int64_t     channel_exit_time;  // channel stop time
    EStatusChannel          status;

    std::shared_ptr<IContext> input_context;        // to do

 public:
    int add_input(std::shared_ptr<PacketQueue> input);
    int del_input(std::shared_ptr<PacketQueue> input);
    // int can_use_input();
    int add_output(std::shared_ptr<PacketQueue> new_output);
    void list_output(std::vector<PacketQueue*> output_list);
    int del_output(std::shared_ptr<PacketQueue> output);
    // int can_use_output();
    void del_all_output();
    virtual int init_cache();
    int get_cache(SortedCache<std::shared_ptr<IPacket>> msgs);
    void init_user_ctrl(std::shared_ptr<IUserHandler> ctrl);
    int run();
    int cycle() override;
    int on_init();
    int on_process();
    virtual int on_cycle(std::shared_ptr<IPacket> packet);
    /*
    * get packet from other channel's queue. maybe there are multiple input
    */
    void fetch(std::vector<std::shared_ptr<IPacket> > &packets, int timeout_us = -1);
    /*
    * copy packet to other channel's queue
    */
    void dispatch(std::shared_ptr<IPacket> packet);

    /*
    *   get the idle time
    */
    int64_t get_idle_time();

    int on_stop();

    bool check_expire();

    EStatusChannel get_status();
    void set_status(EStatusChannel status);
    std::string get_key();

    void wake_up();

 public:
    // to do, static file cache, like hls
    // int get_segment(const std::string& name, std::shared_ptr<File> segment);

 private:
    /*
    *   static file. like hls/dash
    */
    std::vector<std::shared_ptr<SegmentsCache>> segment_cache_list;
};

class ChannelPool : public ICoroutineHandler {
 public:
    ChannelPool();
    ~ChannelPool();
    std::shared_ptr<Channel> fetch(const std::string& key);
    int fetch_or_create(const std::string& key, std::shared_ptr<Channel> &channel);
    error_t cycle();
    void remove(std::shared_ptr<Channel> Channel);
 private:
    std::map<std::string, std::shared_ptr<Channel>>  pool;
};
}  // namespace tmss

