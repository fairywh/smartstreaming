/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#include "tmss_segment.hpp"
#include <log/log.hpp>
#include <util/util.hpp>

namespace tmss {
const int max_input_queue_size = 1000;
int segment_write_packet(void *opaque, uint8_t *buf, int buf_size) {
    SegmentHandler* handler = static_cast<SegmentHandler*>(opaque);
    return handler->write(buf, buf_size);
}

SegmentsCache::SegmentsCache(int min_segment_size, int max_segment_size) {
    this->min_segment_size = min_segment_size;
    this->max_segment_size = max_segment_size;
    segment_enable = false;
}

void SegmentsCache::init_format(std::shared_ptr<IMux> mux) {
    this->mux = mux;
}

int SegmentsCache::handle_packet(std::shared_ptr<IPacket> packet) {
    // check if it is a new segment
    int ret = error_success;
    if (!segment_enable) {
        // not segment
        return ret;
    }
    if (current_segment->duration_ms > 5 * 1000) {
        // a new file
        std::string file_name = cache_name + to_string<int64_t>(current_segment->first_timestamp);
        std::shared_ptr<File> file = std::make_shared<File>(file_name);
        static_cache->add_file(file_name, file);
        current_segment = std::make_shared<SegmentHandler>(file);   // new segment
        // reset mux
        current_segment->init_format(this->mux);
        current_segment->init_output();
    } else {
    }
    ret = current_segment->handle_packet(packet);
    if (ret != error_success) {
        return ret;
    }

    return ret;
}

SegmentHandler::SegmentHandler(std::shared_ptr<File> input_file) :
        PacketQueue(max_input_queue_size) {
    this->file = input_file;
    current_size = 0;
    duration_ms = 0;
    last_timestamp = 0;
    first_timestamp = 0;
}

void SegmentHandler::init_format(std::shared_ptr<IMux> mux) {
    this->mux = mux;
}

int SegmentHandler::handle_packet(std::shared_ptr<IPacket> packet) {
    int ret = mux->handle_output(packet);
    if (ret != error_success) {
        return ret;
    }

    if (last_timestamp > 0) {
        duration_ms += packet->timestamp() - last_timestamp;
    }
    last_timestamp = packet->timestamp();

    if (first_timestamp > 0) {
        first_timestamp = packet->timestamp();
    }

    return ret;
}

int SegmentHandler::init_output() {
    int out_buf_size = 1024 * 16;
    uint8_t* out_buf = new uint8_t[out_buf_size];
    return mux->init_output(out_buf, out_buf_size,
        this, segment_write_packet,
        static_cast<void*>(context.get()), static_cast<void*>(context.get()));
}

int SegmentHandler::write(uint8_t* buff, int size) {
    int wanted_size = size;
    char* temp = reinterpret_cast<char*>(buff);
    int ret = file.lock()->append(temp, wanted_size);
    if (ret != error_success) {
        return ret;
    }

    current_size += size;

    return ret;
}

std::shared_ptr<IContext> SegmentHandler::get_context() {
    return context;
}

void SegmentHandler::set_context(std::shared_ptr<IContext> ctx) {
    context = ctx;
}
}  // namespace tmss