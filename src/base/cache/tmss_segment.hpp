/*
 * TMSS
 * Copyright (c) 2021 rainwu
 */

#pragma once

#include "tmss_input.hpp"
#include "tmss_output.hpp"
#include "tmss_static.hpp"
#include <format/base/mux.hpp>
#include <format/base/demux.hpp>


namespace tmss {

int segment_write_packet(void *opaque, uint8_t *buf, int buf_size);

class SegmentHandler;
class SegmentsCache {
 public:
    SegmentsCache(int min_segment_size, int max_segment_size);
    virtual ~SegmentsCache() = default;

    void init();

    void set_format(const std::string& format);

    int handle_packet(std::shared_ptr<IPacket> packet);

    void open_segment() { segment_enable = true; }
    void close_segment() { segment_enable = false; }

    void set_input_context(std::shared_ptr<IContext> input_ctx);
    //  void set_output_context(std::shared_ptr<IContext> output_ctx);

    std::shared_ptr<FileCache> get_file_cache();

 private:
    std::shared_ptr<FileCache> static_cache;
    std::shared_ptr<SegmentHandler> current_segment;

    int min_segment_size;
    int max_segment_size;

    bool    segment_enable;

    std::string cache_name;     // maybe streamid

    std::string format;
    std::shared_ptr<IContext> input_context;
    //  std::shared_ptr<IContext> output_context;
};

class SegmentHandler : public PacketQueue,
    public std::enable_shared_from_this<SegmentHandler> {
 public:
    explicit SegmentHandler(std::shared_ptr<File> input_file);
    virtual ~SegmentHandler() = default;

    void init(const std::string& format, std::shared_ptr<IContext> input_context);
    int handle_packet(std::shared_ptr<IPacket> packet);
    //  int init_output(std::shared_ptr<IContext> input_context);
    int write(uint8_t* buff, int size);
    void append_pts(int64_t pts);
    std::shared_ptr<IContext> get_context();
    void set_context(std::shared_ptr<IContext> ctx);

    int get_current_size();

    int64_t get_duration_ms();

 private:
    friend class SegmentsCache;
    std::string file_name;
    std::shared_ptr<IMux> mux;
    std::shared_ptr<IContext> input_context;
    std::shared_ptr<IContext> output_context;
    std::weak_ptr<File> file;

    //  std::shared_ptr<IContext>          context;

    int current_size;    // the size already receive

    int duration_ms;

    int64_t last_timestamp;
    int64_t first_timestamp;
};

}  // namespace tmss

