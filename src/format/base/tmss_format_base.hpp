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
#include <format/context.hpp>
#include <format/mux.hpp>
#include <format/demux.hpp>
#include <format/raw/tmss_format_raw.hpp>
#include <format/packet.hpp>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
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
    virtual int play(std::shared_ptr<IClientConn> conn);
    void set_format(const std::string& format);

 private:
    bool is_send_avheader;

    TmssAVFormatContext* ofmt_ctx;
    struct StreamInfo {
        AVRational time_base;
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

    AVPacket    packet;
};

class TmssAVFormatContext : public IContext {
// to do
 public:
    TmssAVFormatContext();
    virtual ~TmssAVFormatContext() = default;

    AVFormatContext* ctx;
};
}  // namespace tmss

