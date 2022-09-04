/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */
#include <algorithm>
#include "tmss_channel.hpp"
#include <util/timer.hpp>
#include <tmss_user_control.hpp>
#include <log/log.hpp>

namespace tmss {
ChannelMgr::ChannelMgr(std::shared_ptr<ChannelPool> pool) {
    this->pool = pool;
}

ChannelMgr::~ChannelMgr() {
}

int ChannelMgr::fetch_or_create_channel(const std::string& hash_key, std::shared_ptr<Channel>& new_channel) {
    std::shared_ptr<Channel> channel;
    int ret = pool->fetch_or_create(hash_key, channel);
    if (ret != error_success) {
        return ret;
    }

    if (channel->get_status() == EChannelKill) {
        // cannot use
        ret = error_channel_killed;
        // to do
        return ret;
    }
    channel->wake_up();
    new_channel = channel;

    return ret;
}

Channel::Channel() : ICoroutineHandler("channel") {
    idle_at = -1;
    channel_exit_time = -1;
    status = EChannelInit;
    tmss_info("create channel");
}

Channel::Channel(const std::string& key) : ICoroutineHandler("channel") {
    this->key = key;
    idle_at = -1;
    channel_exit_time = -1;
    status = EChannelInit;
    tmss_info("create channel");
}

Channel::~Channel() {
    tmss_info("channel delete");
}

int Channel::add_input(std::shared_ptr<PacketQueue> new_input) {
    tmss_info("channel add input");
    int ret = error_success;
    wake_up();
    channel_exit_time = -1;

    input_queue.push_back(new_input);
    return ret;
}

int Channel::del_input(std::shared_ptr<PacketQueue> input) {
    // delete
    auto iter = std::find(input_queue.begin(), input_queue.end(), input);
    if (iter != input_queue.end()) {
        input_queue.erase(iter);
    }
    if (input_queue.empty() && (idle_at == -1)) {
        // idle
        idle_at = get_cache_time();
        tmss_info("channel idle");
    }

    return error_success;
}

int Channel::add_output(std::shared_ptr<PacketQueue> new_output) {
    tmss_info("channel add output");
    int ret = error_success;
    wake_up();
    channel_exit_time = -1;

    output_queue.push_back(new_output);

    //  if (status == EChannelStart) {
        auto output = std::dynamic_pointer_cast<OutputHandler>(new_output);
        output->init_output(input_context);
    //  }
    return ret;
}

void Channel::list_output(std::vector<PacketQueue*> output_list) {
}

int Channel::del_output(std::shared_ptr<PacketQueue> output) {
    // delete
    auto iter = std::find(output_queue.begin(), output_queue.end(), output);
    if (iter != output_queue.end()) {
        output_queue.erase(iter);
        tmss_info("output delete");
    }

    check_and_sleep();

    return error_success;
}

void Channel::del_all_output() {
    output_queue.clear();
}

int Channel::run() {
    if (status == EChannelStart) {
        tmss_info("already start");
        return error_success;
    }
    int ret = on_init();
    if (ret != 0) {
        tmss_error("channel init error, ret={}", ret);
        return ret;
    }
    status = EChannelStart;
    tmss_info("channel start");
    return start();
}

int Channel::cycle() {
    int ret = error_success;
    ret = on_process();
    if (ret != 0) {
        return ret;
    }
    return ret;
}

int Channel::on_init() {
    int ret = error_success;
    tmss_info("channel init");
    // to do
    for (auto queue : this->input_queue) {
        if (queue) {
            auto input = std::dynamic_pointer_cast<InputHandler>(queue);
            ret = input->init_input();
            if (ret != error_success) {
                tmss_error("input init error,ret={}", ret);
                return ret;
            }
            int ret = input->probe();
            if (ret != error_success) {
                tmss_error("input probe error,ret={}", ret);
                return ret;
            }
            if (input_context == nullptr) {
                input_context = input->get_context();
            }
        }
    }
    for (auto queue : this->output_queue) {
        if (queue) {
            auto output = std::dynamic_pointer_cast<OutputHandler>(queue);
            ret = output->init_output(input_context);
            if (ret != error_success) {
                tmss_error("output init error,ret={}", ret);
                return ret;
            }
        }
    }
    // init output to cache
    if (segment_cache) {
        segment_cache->set_input_context(input_context);
        /*ret = cache->init_format();
        if (ret != error_success) {
            tmss_error("cache init error,ret={}", ret);
            return ret;
        }   //*/
    }

    for (auto queue : this->input_queue) {
        if (queue) {
            auto input = std::dynamic_pointer_cast<InputHandler>(queue);
            input->run();
        }
    }
    return ret;
}

int Channel::on_process() {
    int ret = this->user_ctrl->handle_cycle(std::dynamic_pointer_cast<Channel>(shared_from_this()));

    // some condition: 1. push and play, 2. origin and play, 3. push and forward, 4. origin and forward
    ret = on_stop();

    return ret;
}

int Channel::on_cycle(std::shared_ptr<IPacket> packet) {
    int ret = error_success;
    // segment, like flv->hls
    if (segment_cache) {
        ret = segment_cache->handle_packet(packet);
        if (ret != error_success) {
            tmss_error("segment error, ret={}", ret);
            return ret;
        }
    }
    return ret;
}

void Channel::fetch(std::vector<std::shared_ptr<IPacket>> &packets, int timeout_us) {
    int ret = error_success;
    for (auto input : input_queue) {
        std::shared_ptr<IPacket> packet;
        ret = input->dequeue(packet, timeout_us);
        if (ret != error_success) {
            tmss_error("dequeue error, ret={}", ret);
            return;
        }
        if (packet) {
            packets.push_back(packet);
        }
    }
}

void Channel::dispatch(std::shared_ptr<IPacket> packet) {
    if (!packet) {
        return;
    }
    for (auto output : output_queue) {
        output->enqueue(packet);
    }
}

void Channel::init_user_ctrl(std::shared_ptr<IUserHandler> ctrl) {
    user_ctrl = ctrl;
}

int Channel::init_cache() {
    int ret = error_success;

    return ret;
}

int Channel::get_cache(SortedCache<std::shared_ptr<IPacket>> msgs) {
    int ret = error_success;

    return ret;
}

int64_t Channel::get_idle_time() {
    return (idle_at > 0) ? (get_cache_time() - idle_at) : -1;
}

int Channel::on_stop() {
    int ret = error_success;
    status = EChannelInit;
    for (auto queue : input_queue) {
        auto input = std::dynamic_pointer_cast<InputHandler>(queue);
        input->set_stop();
        tmss_info("set input stop");
    }
    for (auto queue : output_queue) {
        auto output = std::dynamic_pointer_cast<OutputHandler>(queue);
        output->send_no_msg();
        output->set_stop();
        tmss_info("set output stop");
    }
    channel_exit_time = get_cache_time();

    tmss_info("channel stop");
    return ret;
}

bool Channel::check_expire() {
    /* check stop time */
    if ((channel_exit_time > 0) && (get_cache_time() - channel_exit_time > 60 * 1000 * 1000)) {
        return true;
    } else {
        return false;
    }
}

EStatusChannel Channel::get_status() {
    return status;
}
void Channel::set_status(EStatusChannel status) {
    this->status = status;
}

std::string Channel::get_key() {
    return key;
}

void Channel::wake_up() {
    idle_at = -1;
}

void Channel::check_and_sleep() {
    if ((idle_at == -1) && output_queue.empty() && !segment_cache) {
        tmss_info("output empty");
        // idle
        bool no_push = true;
        for (auto queue : this->input_queue) {
            if (queue) {
                auto input = std::dynamic_pointer_cast<InputHandler>(queue);
                if (input->get_type() == EInputPublish) {
                    no_push = false;
                }
            }
        }
        if (no_push) {
            idle_at = get_cache_time();
            tmss_info("channel idle");
        }
    }
}

int Channel::add_segemt_cache(std::shared_ptr<SegmentsCache> cache) {
    int ret = error_success;

    wake_up();
    channel_exit_time = -1;
    this->segment_cache = cache;

    return ret;
}

int Channel::del_segment_cache() {
    this->segment_cache = nullptr;

    check_and_sleep();

    return error_success;
}

ChannelPool::ChannelPool() : ICoroutineHandler("channel_pool") {
}

ChannelPool::~ChannelPool() {
}

std::shared_ptr<Channel> ChannelPool::fetch(const std::string& key) {
    std::shared_ptr<Channel> channel;
    auto ch = pool.find(key);
    if (ch != pool.end()) {
        channel = ch->second;
        // check if the channel can be used
        if (channel->get_status() == EChannelKill) {
            channel = nullptr;
        }
    } else {
    }
    return channel;
}

int ChannelPool::fetch_or_create(const std::string& key, std::shared_ptr<Channel> &channel) {
    int ret = error_success;
    auto ch = pool.find(key);
    if (ch != pool.end()) {
        tmss_info("get the channel");
        channel = ch->second;
        // check if the channel can be used
        if (channel->get_status() == EChannelKill) {
            // to do
            channel = std::make_shared<Channel>(key);
            pool.erase(channel->get_key());
            pool.insert(std::make_pair(key, channel));
        }
    } else {
        tmss_info("create a new channel");
        channel = std::make_shared<Channel>(key);
        pool.insert(std::make_pair(key, channel));
    }
    return ret;
}

error_t ChannelPool::cycle() {
    while (true) {
        tmss_info("pool_size={}", pool.size());
        for (auto iter : pool) {
            std::shared_ptr<Channel> channel = iter.second;
            if (channel->check_expire()) {
                channel->set_status(EChannelKill);  // make sure the channel cannot be used during destructing
                remove(channel);
            } else {
                tmss_info("channel_key={}", channel->get_key());
            }
        }
        st_sleep(5);
    }

    return error_success;
}

void ChannelPool::remove(std::shared_ptr<Channel> channel) {
    tmss_info("channel remove");
    pool.erase(channel->get_key());
}

}  // namespace tmss

