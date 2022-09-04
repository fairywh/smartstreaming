/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng.
 *
 * =====================================================================================
 */
#pragma once

#include <memory>
#include <io/io_buffer.hpp>
#include <format/base/context.hpp>
#include <format/base/packet.hpp>
#include <format/base/frame.hpp>
#include <net/tmss_conn.hpp>

namespace tmss {
/*
*   similar with AVInputFormat
*/
class IDeMux {
 public:
    IDeMux() = default;
    virtual ~IDeMux() = default;

 public:
    virtual int init_input(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context) = 0;

    virtual int handle_input(std::shared_ptr<IPacket>& packet) = 0;

    virtual int handle_input(std::shared_ptr<IFrame>& frame) = 0;

    /*virtual int ingest(const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& param,
        std::shared_ptr<IClientConn> conn) = 0;     //  */

    virtual int on_ingest(int content_length, const std::string& data_header) = 0;

    virtual int publish(std::shared_ptr<IClientConn> conn) = 0;

    virtual int get_total_length() { return -1;}

    virtual int read_packet(char* buf, int size);

 public:
    virtual std::shared_ptr<IContext> get_context();
    virtual void set_context(std::shared_ptr<IContext> context);
    virtual std::shared_ptr<Buffer> get_buffer() { return buffer; }

 protected:
    std::shared_ptr<IContext> ctx;
    void *opaque;
    //  buffer
    std::shared_ptr<Buffer> buffer;
    int (*read_packet_func)(void *opaque, uint8_t *buf, int buf_size);  // io
};

}  // namespace tmss
