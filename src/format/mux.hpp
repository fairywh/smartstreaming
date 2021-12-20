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
#include <format/context.hpp>
#include <format/packet.hpp>
#include <tmss_conn.hpp>

namespace tmss {
class IMux {
 public:
    IMux() = default;
    virtual ~IMux() = default;

 public:
    virtual int init_output(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context, void* output_context) = 0;

    virtual int handle_output(std::shared_ptr<IPacket> packet) = 0;

    virtual int play(std::shared_ptr<IClientConn> conn) = 0;

    virtual int forward(const std::string& forward_url,
        std::shared_ptr<IClientConn> conn) = 0;

    virtual int send_status(int status) = 0;

 public:
    virtual std::shared_ptr<IContext> get_context();
    virtual void set_context(std::shared_ptr<IContext> context);

 protected:
    std::shared_ptr<IContext> ctx;
    void *opaque;
    //  buffer
    std::shared_ptr<Buffer> buffer;
    int (*write_packet_func)(void *opaque, uint8_t *buf, int buf_size);  // io
};

}  // namespace tmss
