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
#include <format/context.hpp>
#include <format/mux.hpp>
#include <format/demux.hpp>
#include <format/packet.hpp>

namespace tmss {
class RawDeMux : virtual public IDeMux {
 public:
    RawDeMux();
    virtual ~RawDeMux();
    virtual int init_input(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context);
    virtual int handle_input(std::shared_ptr<IPacket>& packet);
    virtual int on_ingest(int content_length, const std::string& data_header);
    virtual int publish(std::shared_ptr<IClientConn> conn);

    int get_total_length() { return content_length; }

 protected:
    int content_length;     // when it is a static file, it is the file size

    // flush all data to packet
    int flush_read(std::shared_ptr<IPacket>& packet);
};

/*
*   data flow with long connection
*/
class RawMux : virtual public IMux {
 public:
    RawMux();
    virtual ~RawMux();
    virtual int init_output(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context, void* output_context);
    virtual int handle_output(std::shared_ptr<IPacket> packet);
    virtual int play(std::shared_ptr<IClientConn> conn);
    virtual int forward(const std::string& forward_url,
        std::shared_ptr<IClientConn> conn);
    int send_status(int status);

 private:
    // flush all data to output
    int flush_write();
    bool is_send_header;
};

class CommonPacket : public IPacket {
 private:
    char* ptr;  // pointer to raw packet or AVPacket
    int   size;

 public:
    explicit CommonPacket(int max_size);
    ~CommonPacket();
    char* buffer();
    int get_size();
    int64_t timestamp();
    void set_size(int size);
};

class SimplePacket : public IPacket {
 private:
    char* ptr;  // pointer to raw packet or AVPacket
    int   size;

 public:
    SimplePacket(char * buffer, int size);
    ~SimplePacket();
    char* buffer();
    int get_size();
    int64_t timestamp();
};

}  // namespace tmss

