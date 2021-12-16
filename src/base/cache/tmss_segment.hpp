/*
 * TMSS
 * Copyright (c) 2021 rainwu
 */

#pragma once

#include "tmss_input.hpp"
#include "tmss_output.hpp"
#include "tmss_static.hpp"
#include <format/mux.hpp>
#include <format/demux.hpp>


namespace tmss {
class SegmentHandler;
class SegmentsCache {
 public:
    SegmentsCache(int min_segment_size, int max_segment_size);
    virtual ~SegmentsCache() = default;

    void init_format(std::shared_ptr<IMux> mux);

    int handle_packet(std::shared_ptr<IPacket> packet);

    void open_segment() { segment_enable = true; }
    void close_segment() { segment_enable = false; }

 private:
    std::shared_ptr<FileCache> static_cache;
    std::shared_ptr<SegmentHandler> current_segment;

    int min_segment_size;
    int max_segment_size;

    bool    segment_enable;

    std::string cache_name;     // maybe streamid

    std::shared_ptr<IMux> mux;
};

class SegmentHandler : public PacketQueue,
    public std::enable_shared_from_this<SegmentHandler> {
 public:
    explicit SegmentHandler(std::shared_ptr<File> input_file);
    virtual ~SegmentHandler() = default;

    void init_format(std::shared_ptr<IMux> mux);
    int handle_packet(std::shared_ptr<IPacket> packet);
    int init_output();
    int write(uint8_t* buff, int size);

    std::shared_ptr<IContext> get_context();
    void set_context(std::shared_ptr<IContext> ctx);

 private:
    friend class SegmentsCache;
    std::string file_name;
    std::shared_ptr<IMux> mux;
    std::weak_ptr<File> file;

    std::shared_ptr<IContext>          context;

    int current_size;    // the size already receive

    int duration_ms;

    int64_t last_timestamp;
    int64_t first_timestamp;
};

}  // namespace tmss

