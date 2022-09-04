/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#include "tmss_segment.hpp"
#include <log/log.hpp>
#include <util/util.hpp>
#include <format/raw/tmss_format_raw.hpp>
#include <format/ffmpeg/tmss_format_base.hpp>

namespace tmss {
const int max_input_queue_size = 1000;
int segment_write_packet(void *opaque, uint8_t *buf, int buf_size) {
    SegmentHandler* handler = static_cast<SegmentHandler*>(opaque);
    return handler->write(buf, buf_size);
}

SegmentsCache::SegmentsCache(int min_segment_size, int max_segment_size) {
    this->min_segment_size = min_segment_size;
    this->max_segment_size = max_segment_size;
    segment_enable = true;

    static_cache = std::make_shared<FileCache>();
}

void SegmentsCache::init() {
}

void SegmentsCache::set_format(const std::string& format) {
    this->format = format;
}

void SegmentsCache::set_input_context(std::shared_ptr<IContext> input_ctx) {
    this->input_context = input_ctx;
}

/*void SegmentsCache::set_output_context(std::shared_ptr<IContext> output_ctx) {
    this->output_context = output_ctx;
}   //*/

std::shared_ptr<FileCache> SegmentsCache::get_file_cache() {
    return static_cache;
}

int SegmentsCache::handle_packet(std::shared_ptr<IPacket> packet) {
    // check if it is a new segment
    int ret = error_success;
    if (!segment_enable) {
        // not segment
        return ret;
    }

    if (!current_segment) {
        std::string file_name = cache_name + to_string<int64_t>(packet->timestamp());
        std::shared_ptr<File> file = std::make_shared<File>(file_name);
        file->init_buffer(16 * 1024);
        static_cache->add_file(file_name, file);
        current_segment = std::make_shared<SegmentHandler>(file);
        current_segment->init(this->format, this->input_context);
        tmss_info("create first segment");
    }

    current_segment->append_pts(packet->timestamp());
    if ((current_segment->get_duration_ms() >= 5 * 1000) && (packet->is_key_frame())) {
        //  flush manifest
        tmss_info("finish the file {}", current_segment->file_name.c_str());
        // a new file
        std::string file_name = cache_name + to_string<int64_t>(current_segment->first_timestamp);
        std::shared_ptr<File> file = std::make_shared<File>(file_name);
        file->init_buffer(16 * 1024);
        static_cache->add_file(file_name, file);
        tmss_info("create and add new file {}",
            file_name.c_str());
        current_segment = std::make_shared<SegmentHandler>(file);   // new segment
        // reset mux
        current_segment->init(this->format, this->input_context);
        //  current_segment->set_context(this->output_context);
        //  current_segment->init_output(this->input_context);
    } else {
    }
    assert(current_segment);
    ret = current_segment->handle_packet(packet);
    tmss_info("segment_size={}, duration={}", current_segment->get_current_size(), current_segment->get_duration_ms());
    if (ret != error_success) {
        tmss_error("handle packet error, ret={}", ret);
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

void SegmentHandler::init(const std::string& format,
        std::shared_ptr<IContext> input_context) {
    this->mux = nullptr;
    this->input_context = input_context;
    std::shared_ptr<IContext> context;      //  output
    if (format == "RAW") {  // to do
        this->mux = std::make_shared<RawMux>();
        this->output_context = std::make_shared<IContext>();
    } else if (format == "mpegts") {
        this->mux = std::make_shared<MpegTsMux>();
        this->output_context = std::make_shared<TmssAVFormatContext>(false);
    } else {
        this->mux = std::make_shared<FlvMux>();
        this->output_context = std::make_shared<TmssAVFormatContext>(false);
    }

    int segment_buf_size = 1024 * 16;
    uint8_t* segment_buf = new uint8_t[segment_buf_size];
    this->mux->init_output(segment_buf, segment_buf_size,
        this, segment_write_packet, static_cast<void*>(input_context.get()), static_cast<void*>(output_context.get()));

    tmss_info("init success, format={}", format.c_str());
}

int SegmentHandler::handle_packet(std::shared_ptr<IPacket> packet) {
    int ret = mux->handle_output(packet);
    if (ret != error_success) {
        tmss_error("segment handle packet error, ret={}", ret);
        return ret;
    }

    last_timestamp = packet->timestamp();

    if (first_timestamp == 0) {
        first_timestamp = packet->timestamp();
    }

    return ret;
}

/*int SegmentHandler::init_output(std::shared_ptr<IContext> input_context) {
    int out_buf_size = 1024 * 16;
    uint8_t* out_buf = new uint8_t[out_buf_size];
    return mux->init_output(out_buf, out_buf_size,
        this, segment_write_packet,
        static_cast<void*>(input_context.get()), static_cast<void*>(context.get()));
}   //*/

int SegmentHandler::write(uint8_t* buff, int size) {
    int wanted_size = size;
    char* temp = reinterpret_cast<char*>(buff);
    int ret = file.lock()->append(temp, wanted_size);
    if (ret != error_success) {
        tmss_error("file append error,ret={}, wanted_size={}", ret, wanted_size);
        return -1;
    }

    tmss_info("segment write, size={}", size);
    current_size += size;

    return size;
}

void SegmentHandler::append_pts(int64_t pts) {
    if (last_timestamp > 0) {
        duration_ms += pts - last_timestamp;
    }
}

std::shared_ptr<IContext> SegmentHandler::get_context() {
    return output_context;
}

void SegmentHandler::set_context(std::shared_ptr<IContext> ctx) {
    output_context = ctx;
}

int SegmentHandler::get_current_size() {
    return current_size;
}

int64_t SegmentHandler::get_duration_ms() {
    return duration_ms;
}
}  // namespace tmss