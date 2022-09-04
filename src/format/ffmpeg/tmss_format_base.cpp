/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include <format/ffmpeg/tmss_format_base.hpp>
#include "http_stack.hpp"
#include <log/log.hpp>
#include <util/util.hpp>

extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
}

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

    ifmt_ctx->fmt_ctx = avformat_alloc_context();
    ifmt_ctx->fmt_ctx->pb = in_avio_ctx;

    return 0;
}

int BaseDeMux::handle_input(std::shared_ptr<IPacket>& packet) {
    AVPacket av_packet;
    int ret = av_read_frame(ifmt_ctx->fmt_ctx, &av_packet);
    tmss_info("av_read_frame, ret={}", ret);
    if (ret < 0) {
        tmss_error("read frame failed or EOF.ret={}", ret);
        ret = error_ffmpeg_read;
        return ret;
    }

    tmss_info("get avpacket, stream_index={}, size={}, pts={}",
        av_packet.stream_index, av_packet.size, av_packet.pts);
    // set packet
    std::shared_ptr<TmssAVPacket> tmss_packet = std::make_shared<TmssAVPacket>(av_packet);
    tmss_packet->stream_index = av_packet.stream_index;
    packet = tmss_packet;
    return 0;
}

int BaseDeMux::handle_input(std::shared_ptr<IFrame>& frame) {
    AVPacket av_packet;
    int ret = av_read_frame(ifmt_ctx->fmt_ctx, &av_packet);
    if (ret < 0) {
        tmss_error("read frame failed or EOF.ret={}", ret);
        ret = error_ffmpeg_read;
        return ret;
    }

    tmss_info("get avpacket, stream_index={}, size={}, pts={}",
        av_packet.stream_index, av_packet.size, av_packet.pts);

    int stream_index = av_packet.stream_index;
    ret = avcodec_send_packet(ifmt_ctx->codec_ctx_group[stream_index], &av_packet);
    if (ret < 0) {
        tmss_error("Error decoding: %d\n", ret);
        return ret;
    }

    AVFrame av_frame;
    ret = avcodec_receive_frame(ifmt_ctx->codec_ctx_group[stream_index], &av_frame);
    if (ret < 0) {
        tmss_error("Error decoding: %d\n", ret);
        return ret;
    }

    tmss_info("decode a avframe, pts={}", av_frame.pts);
    std::shared_ptr<TmssAVFrame> tmss_frame = std::make_shared<TmssAVFrame>(av_frame);
    tmss_frame->stream_index = stream_index;
    frame = tmss_frame;
    return 0;
}

int open_bitstream_filter(AVStream *stream, AVBSFContext **bsf_ctx, const char *name) {
    int ret = 0;
    const AVBitStreamFilter *filter = av_bsf_get_by_name(name);
    if (!filter) {
        ret = -1;
        tmss_error("Unknow bitstream filter");
    }
    if ((ret = av_bsf_alloc(filter, bsf_ctx) < 0)) {
        tmss_error("av_bsf_alloc failed");
        return ret;
    }
    if ((ret = avcodec_parameters_copy((*bsf_ctx)->par_in, stream->codecpar)) < 0) {
        tmss_error("avcodec_parameters_copy failed, ret={}", ret);
        return ret;
    }
    if ((ret = av_bsf_init(*bsf_ctx)) < 0) {
        tmss_error("av_bsf_init failed, ret={}", ret);
        return ret;
    }
    return ret;
}

int BaseDeMux::on_ingest(int content_length, const std::string& data_header) {
    int ret = error_success;
    tmss_info("init avformat");
    // init avformat
    AVDictionary* opts = NULL;
    av_dict_set(&opts, "timeout", "2000000", 0);
    ret = avformat_open_input(&ifmt_ctx->fmt_ctx, NULL, NULL, &opts);
    if (ret < 0) {
        tmss_error("avformat_open_input failed.ret={}", ret);
        ret = error_ffmpeg_init_input;
        return ret;
    }
    tmss_info("open input success.");
    ret = avformat_find_stream_info(ifmt_ctx->fmt_ctx, &opts);
    if (ret < 0) {
        tmss_error("avformat_find_stream_info failed.ret={}", ret);
        ret = error_ffmpeg_init_input;
        return ret;
    }
    tmss_info("ingest success");

    ifmt_ctx->video_stream_index = 0;
    for (size_t i = 0; i < ifmt_ctx->fmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->fmt_ctx->streams[i];
        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!dec) {
            tmss_error("Failed to find decoder for stream #%u", i);
            ret = error_ffmpeg_init_input;
            return ret;
        }

        AVCodecContext* codec_ctx = avcodec_alloc_context3(dec);
        ifmt_ctx->codec_ctx_group.push_back(codec_ctx);
        if (!codec_ctx) {
            tmss_error("Failed to allocate the decoder context for stream #%u", i);
            ret = error_ffmpeg_init_input;
            return ret;
        }

        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret != 0) {
            tmss_error("Can't copy decoder context, ret={}", ret);
            ret = error_ffmpeg_init_input;
            return ret;
        }

        ret = avcodec_open2(codec_ctx, dec, NULL);
        if (ret < 0) {
            tmss_error("Can't open decoder, ret={}", ret);
            return ret;
        }

        codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx->fmt_ctx, stream, NULL);

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            ifmt_ctx->video_stream_index = i;
            ifmt_ctx->is_annexb =
                stream->codecpar->codec_tag != 0;
            tmss_info("codec_tag={}", stream->codecpar->codec_tag);
        }

        tmss_info("frame rate={},{}",
            codec_ctx->framerate.num, codec_ctx->framerate.den);
    }

    if (!ifmt_ctx->is_annexb) {
        if ((ret = open_bitstream_filter(ifmt_ctx->fmt_ctx->streams[ifmt_ctx->video_stream_index],
            &ifmt_ctx->bsf_ctx,
            "h264_mp4toannexb")) < 0) {
           tmss_error("open_bitstream_filter failed, ret={}", ret);
           return ret;
        }
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
    RawMux::init_output(buffer, buffer_size, opaque, write_packet, input_context, output_context);

    ofmt_ctx = reinterpret_cast<TmssAVFormatContext*>(output_context);
    AVIOContext *out_avio_ctx = avio_alloc_context(buffer, buffer_size,
        1, opaque,
        NULL, write_packet, NULL);
    ofmt_ctx->fmt_ctx = NULL;
    int ret = avformat_alloc_output_context2(&ofmt_ctx->fmt_ctx, NULL, format.c_str(), NULL);
    if (ret < 0) {
        tmss_error("avformat_alloc_output_context2 failed.ret={}", ret);
        ret = error_ffmpeg_init_output;
        return ret;
    }
    ofmt_ctx->fmt_ctx->pb = out_avio_ctx;

    if (input_context) {
        ifmt_ctx = reinterpret_cast<TmssAVFormatContext*>(input_context);
        if (!ifmt_ctx) {
            tmss_error("context invalid");
            ret = error_ffmpeg_init_output;
            return ret;
        }
        tmss_info("streams_num={}", ifmt_ctx->fmt_ctx->nb_streams);
        for (size_t i = 0; i < ifmt_ctx->fmt_ctx->nb_streams; i++) {
            AVStream *out_stream;
            AVStream *in_stream = ifmt_ctx->fmt_ctx->streams[i];
            AVCodecParameters *in_codecpar = in_stream->codecpar;

            if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
                in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
                in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                continue;
            }

            /* we choose transcoding to same codec */
            const AVCodec *encoder =
                avcodec_find_encoder(ifmt_ctx->codec_ctx_group[i]->codec_id);

            if (!encoder) {
                tmss_error("Failed to find encoder for stream {}, {}",
                    i, ifmt_ctx->codec_ctx_group[i]->codec_id);
                ret = error_ffmpeg_init_output;
                return ret;
            }
            AVCodecContext* codec_ctx = avcodec_alloc_context3(encoder);
            ofmt_ctx->codec_ctx_group.push_back(codec_ctx);

            if (!codec_ctx) {
                tmss_error("Failed to allocate the encoder context for stream %u", i);
                ret = error_ffmpeg_init_input;
                return ret;
            }
            //  stream_info[i].time_base = in_stream->time_base;

            out_stream = avformat_new_stream(ofmt_ctx->fmt_ctx, NULL);
            if (!out_stream) {
                tmss_error("Failed allocating output stream");
                ret = error_ffmpeg_init_output;
                return ret;
            }

            /* These properties can be changed for output
             * streams easily using filters */
            if (ifmt_ctx->codec_ctx_group[i]->codec_type == AVMEDIA_TYPE_VIDEO) {
                codec_ctx->height = ifmt_ctx->codec_ctx_group[i]->height;
                codec_ctx->width = ifmt_ctx->codec_ctx_group[i]->width;
                codec_ctx->sample_aspect_ratio = ifmt_ctx->codec_ctx_group[i]->sample_aspect_ratio;
                /* take first format from list of supported formats */
                if (encoder->pix_fmts)
                    codec_ctx->pix_fmt = encoder->pix_fmts[0];
                else
                    codec_ctx->pix_fmt = ifmt_ctx->codec_ctx_group[i]->pix_fmt;
                /* video time_base can be set to whatever is handy and supported by encoder */
                codec_ctx->time_base = av_inv_q(ifmt_ctx->codec_ctx_group[i]->framerate);

                tmss_info("time_base={},{}, framerate={},{}",
                    codec_ctx->time_base.num, codec_ctx->time_base.den,
                    ifmt_ctx->codec_ctx_group[i]->framerate.num,
                    ifmt_ctx->codec_ctx_group[i]->framerate.den);
            } else {
                codec_ctx->sample_rate = ifmt_ctx->codec_ctx_group[i]->sample_rate;
                codec_ctx->channel_layout = ifmt_ctx->codec_ctx_group[i]->channel_layout;
                codec_ctx->channels =
                    av_get_channel_layout_nb_channels(codec_ctx->channel_layout);
                /* take first format from list of supported formats */
                codec_ctx->sample_fmt = encoder->sample_fmts[0];
                codec_ctx->time_base = (AVRational){1, codec_ctx->sample_rate};

                tmss_info("time_base={},{}",
                    codec_ctx->time_base.num, codec_ctx->time_base.den);
            }

            if (ofmt_ctx->fmt_ctx->flags & AVFMT_GLOBALHEADER)
                codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            stream_info[i].time_base_in = out_stream->time_base = codec_ctx->time_base;
            if (format == "mpegts") {
                stream_info[i].time_base_out = stream_info[i].time_base_in;
                stream_info[i].time_base_out.den = stream_info[i].time_base_out.den * 90;
            } else {
                stream_info[i].time_base_out = stream_info[i].time_base_in;
            }

            /* Third parameter can be used to pass settings to encoder */
            ret = avcodec_open2(codec_ctx, encoder, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
                return ret;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, codec_ctx);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
                return ret;
            }

            /*
            ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
            if (ret < 0) {
                tmss_error("Failed to copy codec parameters,ret={}", ret);
                ret = error_ffmpeg_init_output;
                return ret;
            }
            out_stream->codecpar->codec_tag = 0;
            //*/
        }
    }

    //  out_ration.num = 1;
    //  out_ration.den = 1000;

    av_opt_set(static_cast<void*>(ofmt_ctx->fmt_ctx->priv_data), "pcr_period", "20", 0);
    //  av_opt_set((void*)ofmt_ctx->fmt_ctx->priv_data, "muxrate", "4000000", 0);
    return error_success;
}

int BaseMux::handle_output(std::shared_ptr<IPacket> packet) {
    // to do
    AVFormatContext * fmt_ctx = ofmt_ctx->fmt_ctx;
    int ret = error_success;
    if (!is_send_avheader) {
        ret = avformat_write_header(fmt_ctx, NULL);
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
        stream_info[av_packet.stream_index].time_base_in,
        stream_info[av_packet.stream_index].time_base_out);
    int64_t dts = av_rescale_q(av_packet.dts,
        stream_info[av_packet.stream_index].time_base_in,
        stream_info[av_packet.stream_index].time_base_out);

    av_packet.pts = pts;
    av_packet.dts = dts;

    //  filter
    if (!ifmt_ctx->is_annexb  && (ifmt_ctx->video_stream_index == av_packet.stream_index)) {
        if (ret = av_bsf_send_packet(ifmt_ctx->bsf_ctx, &av_packet) < 0) {
            tmss_error("av_bsf_send_packet failed, ret={}", ret);
            return ret;
        }
        while ((ret = av_bsf_receive_packet(ifmt_ctx->bsf_ctx, &av_packet) == 0)) {
            ret = av_write_frame(fmt_ctx, &av_packet);
            if (ret < 0) {
                std::string temp;
                char errbuf[128];
                if (av_strerror(ret, errbuf, sizeof(errbuf)) < 0) {
                    temp = strerror(AVUNERROR(ret));
                }
                tmss_error("write frame failed.ret={},{}", ret, temp.c_str());
                ret = error_ffmpeg_write;
                av_packet_unref(&av_packet);
                return ret;
            }
            av_packet_unref(&av_packet);
        }
    } else {
        ret = av_write_frame(fmt_ctx, &av_packet);
        if (ret < 0) {
            std::string temp;
            char errbuf[128];
            if (av_strerror(ret, errbuf, sizeof(errbuf)) < 0) {
                temp = strerror(AVUNERROR(ret));
            }
            tmss_error("write frame failed.ret={},{}", ret, temp.c_str());
            ret = error_ffmpeg_write;
            av_packet_unref(&av_packet);
            return ret;
        }
    }

    return 0;
}

int BaseMux::handle_output(std::shared_ptr<IFrame> frame) {
    // to do
    int ret = error_success;
    if (!is_send_avheader) {
        ret = avformat_write_header(ofmt_ctx->fmt_ctx, NULL);
        if (ret != 0) {
            tmss_error("write header failed, ret={}", ret);
            ret = error_ffmpeg_write_header;
            return ret;
        }
        is_send_avheader = true;
    }


    // get packet
    std::shared_ptr<TmssAVFrame> tmss_frame =
        std::dynamic_pointer_cast<TmssAVFrame>(frame);
    AVFrame av_frame = tmss_frame->frame;
    int stream_index = tmss_frame->stream_index;
    ret = avcodec_send_frame(ofmt_ctx->codec_ctx_group[stream_index], &av_frame);
    if (ret < 0) {
        tmss_error("Error sending a frame for encoding\n");
        return ret;
    }

    AVPacket av_packet;
    ret = avcodec_receive_packet(ofmt_ctx->codec_ctx_group[stream_index], &av_packet);
    if (ret < 0) {
        tmss_error("Error encoding\n");
        return ret;
    }
    av_packet.stream_index = stream_index;
    tmss_info("write avpacket, stream_index={}, size={}, pts={}",
        av_packet.stream_index, av_packet.size, av_packet.pts);
    // remux and codec
    int64_t pts = av_rescale_q(av_packet.pts,
        stream_info[av_packet.stream_index].time_base_in,
        stream_info[av_packet.stream_index].time_base_out);
    int64_t dts = av_rescale_q(av_packet.dts,
        stream_info[av_packet.stream_index].time_base_in,
        stream_info[av_packet.stream_index].time_base_out);

    av_packet.pts = pts;
    av_packet.dts = dts;

    ret = av_write_frame(ofmt_ctx->fmt_ctx, &av_packet);
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

bool TmssAVPacket::is_key_frame() {
    tmss_info("frame_type={}", packet.flags);
    return packet.flags & AV_PKT_FLAG_KEY;
}

TmssAVFrame::TmssAVFrame(AVFrame frame) {
    this->frame = frame;
}

char* TmssAVFrame::buffer() {
    return reinterpret_cast<char*>(frame.data);
}

int TmssAVFrame::get_size() {
    return 0;
}

int64_t TmssAVFrame::timestamp() {
    return 0;
}

TmssAVFormatContext::TmssAVFormatContext() {
    fmt_ctx = NULL;
    is_transcode = false;
}

TmssAVFormatContext::TmssAVFormatContext(bool is_codec) {
    fmt_ctx = NULL;
    is_transcode = is_codec;
}

std::shared_ptr<IMux> create_mux_by_ext(const std::string& ext) {
    std::shared_ptr<IMux> muxer;
    if (ext == "flv") {
        muxer = std::make_shared<FlvMux>();
    } else if (ext == "ts") {
         muxer = std::make_shared<MpegTsMux>();
    } else {
        muxer = std::make_shared<RawMux>();
    }

    return muxer;
}

std::shared_ptr<IDeMux> create_demux_by_format(const std::string& format) {
    std::shared_ptr<IDeMux> demuxer;
    if (format == "flv") {
        demuxer = std::make_shared<FlvDeMux>();
    } else if (format == "ts") {
        demuxer = std::make_shared<MpegTsDeMux>();
    } else {
        demuxer = std::make_shared<RawDeMux>();
    }

    return demuxer;
}

std::shared_ptr<IContext> create_context_by_ext(const std::string& ext) {
    std::shared_ptr<IContext> context;
    if (ext == "flv") {
        context = std::make_shared<TmssAVFormatContext>(false);
    } else if (ext == "ts") {
        context = std::make_shared<TmssAVFormatContext>(false);
    } else {
        context = std::make_shared<IContext>();
    }
    return context;
}

std::shared_ptr<IContext> create_context_by_format(const std::string& format) {
    std::shared_ptr<IContext> context;
    if (format == "flv") {
        context = std::make_shared<TmssAVFormatContext>(false);
    } else if (format == "ts") {
        context = std::make_shared<TmssAVFormatContext>(false);
    } else {
        context = std::make_shared<IContext>();
    }
    return context;
}
}  // namespace tmss

