/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#include "tmss_cache.hpp"
#include <log/log.hpp>

namespace tmss {
PacketQueue::PacketQueue(int size) : max_size(size) {
    cond = st_cond_new();
}

PacketQueue::~PacketQueue() {
    st_cond_destroy(cond);
}

int PacketQueue::enqueue(std::shared_ptr<IPacket> packet) {
    int ret = error_success;
    queue.push(packet);
    st_cond_broadcast(cond);
    tmss_info("enqueue a packet, size={}", packet->get_size());
    return ret;
}

int PacketQueue::dequeue(std::shared_ptr<IPacket> &packet, int timeout_us) {
    int ret = error_success;
    if (queue.empty()) {
        st_cond_timedwait(cond, timeout_us);
        if (queue.empty()) {
            return ret;
        }
    }
    packet = queue.front();
    queue.pop();
    tmss_info("get a packet from queue, size={}", packet->get_size());
    return ret;
}

int PacketQueue::send_no_msg() {
    int ret = error_success;
    st_cond_broadcast(cond);
    return ret;
}

bool PacketQueue::can_use() {
    return true;
}

}  // namespace tmss

