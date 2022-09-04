/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#pragma once
#include <map>
#include <string>
#include <vector>
#include <format/base/context.hpp>
#include <format/base/mux.hpp>
#include <format/base/demux.hpp>
#include <format/raw/tmss_format_raw.hpp>
#include <format/base/frame.hpp>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavcodec/bsf.h"
}


namespace tmss {
class TmssAVFormatContext;
class BaseDeMux : virtual public RawDeMux {
 public:
    BaseDeMux();
    virtual ~BaseDeMux();
    virtual int init_input(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context);
    virtual int handle_input(std::shared_ptr<IPacket>& packet);
    virtual int handle_input(std::shared_ptr<IFrame>& frame);
    /*virtual int ingest(const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& param,
        std::shared_ptr<IClientConn> conn);     //  */
    virtual int on_ingest(int content_length, const std::string& data_header);
    virtual int publish(std::shared_ptr<IClientConn> conn);
    void set_format(const std::string& format);

 private:
    TmssAVFormatContext* ifmt_ctx;
    //  TmssAVCodecContext* codec_ctx;
    std::string format;
};

/*
*   data flow with long connection
*/
class BaseMux : virtual public RawMux {
 public:
    BaseMux();
    ~BaseMux();
    virtual int init_output(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context, void* output_context);
    virtual int handle_output(std::shared_ptr<IPacket> packet);
    virtual int handle_output(std::shared_ptr<IFrame> frame);
    virtual int play(std::shared_ptr<IClientConn> conn);
    void set_format(const std::string& format);

 private:
    bool is_send_avheader;

    TmssAVFormatContext* ifmt_ctx;
    TmssAVFormatContext* ofmt_ctx;
    struct StreamInfo {
        AVRational time_base_in;
        AVRational time_base_out;
    };
    std::map<int, StreamInfo> stream_info;
    AVRational out_ration;
    std::string format;
};

class TmssAVPacket : public IPacket {
// to do
 public:
    explicit TmssAVPacket(AVPacket packet);

    virtual char*  buffer();
    virtual int     get_size();
    virtual int64_t timestamp();
    virtual bool is_key_frame();

    AVPacket    packet;
    int         stream_index;
};

class TmssAVFrame : public IFrame {
// to do
 public:
    explicit TmssAVFrame(AVFrame frame);

    virtual char*  buffer();
    virtual int     get_size();
    virtual int64_t timestamp();

    AVFrame     frame;
    int         stream_index;
};

class TmssAVFormatContext : public IContext {
// to do
 public:
    TmssAVFormatContext();
    explicit TmssAVFormatContext(bool is_codec);
    virtual ~TmssAVFormatContext() = default;

    AVFormatContext* fmt_ctx;
    std::vector<AVCodecContext*> codec_ctx_group;

    bool    is_transcode;
    bool    is_annexb;
    AVBSFContext *bsf_ctx;

    int     video_stream_index;
};

/*class TmssAVCodecContext : public IContext {
// to do
 public:
    TmssAVCodecContext();
    virtual ~TmssAVCodecContext() = default;

    AVCodecContext* codec_ctx;
};  //*/


//  format

class FlvDeMux : virtual public BaseDeMux {
 public:
    FlvDeMux() { this->set_format("flv");}
    ~FlvDeMux() = default;
};

/*
*   data flow with long connection
*/
class FlvMux : virtual public BaseMux {
 public:
    FlvMux() { this->set_format("flv");}
    ~FlvMux() = default;
};

class MpegTsDeMux : virtual public BaseDeMux {
 public:
    MpegTsDeMux() { this->set_format("mpegts");}
    ~MpegTsDeMux() = default;
};

/*
*   data flow with long connection
*/
class MpegTsMux : virtual public BaseMux {
 public:
    MpegTsMux() { this->set_format("mpegts");}
    ~MpegTsMux() = default;
};

std::shared_ptr<IMux> create_mux_by_ext(const std::string& ext);
std::shared_ptr<IDeMux> create_demux_by_format(const std::string& format);

std::shared_ptr<IContext> create_context_by_ext(const std::string& ext);
std::shared_ptr<IContext> create_context_by_format(const std::string& format);
}  // namespace tmss

