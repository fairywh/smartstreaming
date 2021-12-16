/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include <format/base/tmss_format_base.hpp>
#include "http_stack.hpp"
#include <log/log.hpp>
namespace tmss {
const int max_bufer_size = 1 * 1024 * 1024;
BaseDeMux::BaseDeMux() {
}
BaseDeMux::~BaseDeMux() {
}

int BaseDeMux::init_input(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context) {
    tmss_info("init_input");

    RawDeMux::init_input(buffer, buffer_size, opaque, read_packet, input_context);

    ifmt_ctx = reinterpret_cast<TmssAVFormatContext*>(input_context);
    AVIOContext* in_avio_ctx =
        avio_alloc_context(buffer, buffer_size, 0, opaque, read_packet, NULL, NULL);

    ifmt_ctx->ctx = avformat_alloc_context();
    ifmt_ctx->ctx->pb = in_avio_ctx;

    return 0;
}

int BaseDeMux::handle_input(std::shared_ptr<IPacket>& packet) {
    AVPacket av_packet;
    int ret = av_read_frame(ifmt_ctx->ctx, &av_packet);
    if (ret < 0) {
        tmss_error("read frame failed or EOF.ret={}", ret);
        ret = error_ffmpeg_read;
        return ret;
    }

    tmss_info("get avpacket, stream_index={}, size={}, pts={}",
        av_packet.stream_index, av_packet.size, av_packet.pts);
    // set packet
    std::shared_ptr<TmssAVPacket> tmss_packet = std::make_shared<TmssAVPacket>(av_packet);
    packet = tmss_packet;
    return 0;
}

int BaseDeMux::on_ingest(int content_length, const std::string& data_header) {
    int ret = error_success;
    // init avformat
    ret = avformat_open_input(&ifmt_ctx->ctx, NULL, NULL, NULL);
    if (ret < 0) {
        tmss_error("avformat_open_input failed.ret={}", ret);
        ret = error_ffmpeg_init_input;
        return ret;
    }

    ret = avformat_find_stream_info(ifmt_ctx->ctx, NULL);
    if (ret < 0) {
        tmss_error("avformat_find_stream_info failed.ret={}", ret);
        ret = error_ffmpeg_init_input;
        return ret;
    }
    return ret;
}

int BaseDeMux::publish(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;
    // handshake
    return ret;
}

void BaseDeMux::set_format(const std::string& format) {
    this->format = format;
}

BaseMux::BaseMux() {
    is_send_avheader = false;
}

BaseMux::~BaseMux() {
}

int BaseMux::init_output(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context, void* output_context) {
    tmss_info("init_output");

    RawMux::init_output(buffer, buffer_size, opaque, write_packet, input_context, output_context);

    ofmt_ctx = reinterpret_cast<TmssAVFormatContext*>(output_context);
    AVIOContext *out_avio_ctx = avio_alloc_context(buffer, buffer_size,
        1, opaque,
        NULL, write_packet, NULL);

    ofmt_ctx->ctx = NULL;
    int ret = avformat_alloc_output_context2(&ofmt_ctx->ctx, NULL, format.c_str(), NULL);
    if (ret < 0) {
        tmss_error("avformat_alloc_output_context2 failed.ret={}", ret);
        ret = error_ffmpeg_init_output;
        return ret;
    }
    ofmt_ctx->ctx->pb = out_avio_ctx;

    if (input_context) {
        TmssAVFormatContext* ifmt_ctx = reinterpret_cast<TmssAVFormatContext*>(input_context);
        if (!ifmt_ctx) {
            tmss_error("context invalid");
            ret = error_ffmpeg_init_output;
            return ret;
        }
        tmss_info("streams_num={}", ifmt_ctx->ctx->nb_streams);
        for (size_t i = 0; i < ifmt_ctx->ctx->nb_streams; i++) {
            AVStream *out_stream;
            AVStream *in_stream = ifmt_ctx->ctx->streams[i];
            AVCodecParameters *in_codecpar = in_stream->codecpar;

            if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
                in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
                in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                continue;
            }

            stream_info[i].time_base = in_stream->time_base;

            out_stream = avformat_new_stream(ofmt_ctx->ctx, NULL);
            if (!out_stream) {
                tmss_error("Failed allocating output stream");
                ret = error_ffmpeg_init_output;
                return ret;
            }
            ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
            if (ret < 0) {
                tmss_error("Failed to copy codec parameters,ret={}", ret);
                ret = error_ffmpeg_init_output;
                return ret;
            }
            out_stream->codecpar->codec_tag = 0;
        }
    }

    out_ration.num = 1;
    out_ration.den = 1000;

    return error_success;
}

int BaseMux::handle_output(std::shared_ptr<IPacket> packet) {
    // to do
    int ret = error_success;
    if (!is_send_avheader) {
        ret = avformat_write_header(ofmt_ctx->ctx, NULL);
        if (ret != 0) {
            tmss_error("write header failed, ret={}", ret);
            ret = error_ffmpeg_write_header;
            return ret;
        }
        is_send_avheader = true;
    }
    // get packet
    std::shared_ptr<TmssAVPacket> tmss_packet =
        std::dynamic_pointer_cast<TmssAVPacket>(packet);
    AVPacket av_packet = tmss_packet->packet;

    tmss_info("write avpacket, stream_index={}, size={}, pts={}",
        av_packet.stream_index, av_packet.size, av_packet.pts);
    // remux and codec
    int64_t pts = av_rescale_q(av_packet.pts,
        stream_info[av_packet.stream_index].time_base,
        out_ration);
    int64_t dts = av_rescale_q(av_packet.dts,
        stream_info[av_packet.stream_index].time_base,
        out_ration);

    av_packet.pts = pts;
    av_packet.dts = dts;

    ret = av_write_frame(ofmt_ctx->ctx, &av_packet);
    if (ret < 0) {
        tmss_error("write frame failed.ret={}", ret);
        ret = error_ffmpeg_write;
        return ret;
    }
    return 0;
}

int BaseMux::play(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;
    // handshake
    return ret;
}

void BaseMux::set_format(const std::string& format) {
    this->format = format;
}

TmssAVPacket::TmssAVPacket(AVPacket packet) {
    this->packet = packet;
}

char* TmssAVPacket::buffer() {
    return reinterpret_cast<char*>(packet.data);
}

int TmssAVPacket::get_size() {
    return packet.size;
}

int64_t TmssAVPacket::timestamp() {
    return packet.dts;
}

TmssAVFormatContext::TmssAVFormatContext() {
    ctx = NULL;
}

}  // namespace tmss

