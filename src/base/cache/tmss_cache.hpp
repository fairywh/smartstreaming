/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#pragma once

#include <st.h>
#include <memory>
#include <set>
#include <queue>
#include <string>
#include <iostream>
#include <map>
#include <defs/err.hpp>
#include <format/base/packet.hpp>
#include <format/base/frame.hpp>
#include <defs/tmss_def.hpp>

namespace tmss {
template<class T>
class Pool {
 public:
    explicit Pool(int max_size) { this->max_size = max_size; }
    ~Pool() {}
    void add(std::shared_ptr<T> member) { pool.insert(member);}
    void remove(std::shared_ptr<T> member) { pool.erase(member); }

 private:
    std::set<std::shared_ptr<T>>  pool;
    int max_size;
};

template<class T>
class SortedCache {
 public:
    SortedCache() {max_size = 1000;}     //  to do
    ~SortedCache() {}
    SortedCache& operator[](int index) {
        if (index >= max_size) {
            index = 0;
        }
        return cache[index];
    }

 private:
    std::vector<std::shared_ptr<T>> cache;
    int max_size;
};

class PacketQueue {
 public:
    explicit PacketQueue(int size);
    virtual ~PacketQueue();
    virtual int enqueue(std::shared_ptr<IPacket> packet);
    virtual int enqueue(std::shared_ptr<IFrame> frame);
    virtual int dequeue(std::shared_ptr<IPacket> &packet, int timeout_us = -1);
    virtual int dequeue(std::shared_ptr<IFrame> &frame, int timeout_us = -1);
    virtual int send_no_msg();
    virtual bool can_use();

 private:
    std::queue<std::shared_ptr<IPacket>> queue;
    std::queue<std::shared_ptr<IFrame>> frame_queue;
    st_cond_t cond;
    int max_size;
};
}  // namespace tmss

