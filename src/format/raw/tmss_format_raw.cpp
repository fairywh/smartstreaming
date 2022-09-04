/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include <raw/tmss_format_raw.hpp>
#include "http_stack.hpp"
#include <log/log.hpp>
namespace tmss {
const int max_bufer_size = 1 * 1024 * 1024;
RawDeMux::RawDeMux() {
    content_length = 0;
}
RawDeMux::~RawDeMux() {
}

int RawDeMux::init_input(unsigned char *buffer,
    int buffer_size,
    void *opaque, int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
    void * input_context) {

    this->opaque = opaque;
    this->buffer = std::make_shared<Buffer>(reinterpret_cast<char*>(buffer), buffer_size);
    read_packet_func = read_packet;

    tmss_info("init_input");

    return 0;
}

int RawDeMux::handle_input(std::shared_ptr<IPacket>& packet) {
    //  CommonData* data = (CommonData*)packet->buffer();
    // read from buffer

    if (read_packet_func == nullptr) {
        tmss_error("read handler null");
        return -2;
    }
    int min_read_size = 128;
    if (buffer->get_size() < 1) {
        tmss_error("buffer_size error");
        return -1;
    }

    if (min_read_size > buffer->read_left()) {
        int continuous_write_left_size = buffer->continuous_write_left();
        int read_size = read_packet_func(opaque,
            reinterpret_cast<uint8_t*>(buffer->wcurrent()),
            continuous_write_left_size);
        int ret = buffer->seek_write(read_size);
        if (ret != error_success) {
            tmss_error("write error, ret={}", ret);
            //  return ret;
        }
        if (min_read_size > buffer->read_left()) {
            // to do
        } else {
        }
    } else {
    }

    // flush
    flush_read(packet);
    // chunk need to do
    return 0;
}

int RawDeMux::handle_input(std::shared_ptr<IFrame>& frame) {
    return 0;
}

int RawDeMux::on_ingest(int content_length, const std::string& data_header) {
    this->content_length = content_length;
    int ret = error_success;
    // this have some data, (no need, can be delete)
    if (data_header.length() > buffer->write_left()) {
        tmss_error("not enough buffer,need={}/left={}",
            data_header.length(), buffer->write_left());
        ret = error_buffer_not_enough;
        return ret;
    }
    int data_size = data_header.length();
    buffer->write_bytes(data_header.c_str(), data_size);
    tmss_info("data_header, {}, {}",
        data_size, PrintBuffer(data_header.c_str(), (data_size > 100) ? 100 : data_size));
    return ret;
}

int RawDeMux::publish(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;
    // handshake
    return ret;
}

int RawDeMux::flush_read(std::shared_ptr<IPacket>& packet) {
    int ret = error_success;
    if (buffer->read_left() <= 0) {
        // error
        packet = nullptr;
        tmss_error("buffer no read");
        return ret;
    }
    std::shared_ptr<CommonPacket> raw_packet = std::make_shared<CommonPacket>(buffer->read_left());
    // flush, to do, flow control
    int wanted_size = buffer->read_left();
    buffer->read_bytes(raw_packet->buffer(), wanted_size);

    buffer->reset();

    packet = raw_packet;

    tmss_info("flush read packet={},{}", packet->get_size(), packet->buffer());
    return ret;
}

RawMux::RawMux() {
    is_send_header = false;
}

RawMux::~RawMux() {
}

int RawMux::init_output(unsigned char *buffer,
        int buffer_size,
        void *opaque, int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
        void * input_context, void* output_context) {
    this->opaque = opaque;
    this->buffer = std::make_shared<Buffer>(reinterpret_cast<char*>(buffer), buffer_size);
    write_packet_func = write_packet;

    tmss_info("init_output");

    return 0;
}

int RawMux::handle_output(std::shared_ptr<IPacket> packet) {
    //  CommonData* data = (CommonData*)packet->data();
    int left_size = buffer->write_left();
    if (left_size < packet->get_size()) {
        // not enough
        // flush
        flush_write();
        left_size = buffer->write_left();
        if (left_size < packet->get_size()) {
            // to do
            return -1;
        } else {
            int wanted_size = packet->get_size();
            buffer->write_bytes(packet->buffer(), wanted_size);
        }
    } else {
        int wanted_size = packet->get_size();
        buffer->write_bytes(packet->buffer(), wanted_size);
    }

    // flush
    flush_write();
    return 0;
}

int RawMux::handle_output(std::shared_ptr<IFrame> frame) {
    return 0;
}

int RawMux::play(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;
    // handshake
    return ret;
}

int RawMux::forward(const std::string& forward_url,
    std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    std::string request;    // packet forward_url
    conn->write(request.c_str(), request.length());
    char response[1024];
    int read_size = sizeof(response);
    conn->read(response, read_size);
    return ret;
}

int RawMux::send_status(int status) {
    // to do
    int ret = error_success;

    if (is_send_header) {
        return ret;
    }
    // send 200 or 404
    std::string result;
    result = "HTTP/1.1 ";
    result += http_status_detail[status];
    result += "\r\n\r\n";
    char *temp = const_cast<char*>(result.c_str());
    if (write_packet_func) {
        write_packet_func(opaque, reinterpret_cast<uint8_t*>(temp), result.length());
    } else {
        tmss_error("write handler null");
    }

    is_send_header = true;

    return ret;
}

int RawMux::flush_write() {
    int ret = error_success;
    // to do, flow control
    if (write_packet_func == nullptr) {
        tmss_error("write handler null");
        return ret;
    }

    tmss_info("flush write buffer={},{}", buffer->rcurrent(), buffer->read_left());

    int write_size = write_packet_func(opaque,
        reinterpret_cast<uint8_t*>(buffer->rcurrent()), buffer->continuous_read_left());
    buffer->seek_read(write_size);
    if (buffer->read_left() > 0) {
        write_size = write_packet_func(opaque,
            reinterpret_cast<uint8_t*>(buffer->rcurrent()), buffer->read_left());
        buffer->reset();
    }

    return ret;
}

CommonPacket::CommonPacket(int max_size) {
    size = max_size;
    ptr = new char[size];
}

CommonPacket::~CommonPacket() {
    delete []ptr;
}

char* CommonPacket::buffer() {
    return this->ptr;
}

int CommonPacket::get_size() {
    return this->size;
}

int64_t CommonPacket::timestamp() {
    return 0;
}

void CommonPacket::set_size(int size) {
    this->size = size;
}

bool CommonPacket::is_key_frame() {
    return false;
}

SimplePacket::SimplePacket(char * buffer, int size) {
    this->size = size;
    ptr = buffer;
}

SimplePacket::~SimplePacket() {
}

char* SimplePacket::buffer() {
    return this->ptr;
}

int SimplePacket::get_size() {
    return this->size;
}

int64_t SimplePacket::timestamp() {
    return 0;
}

bool SimplePacket::is_key_frame() {
    return false;
}
}  // namespace tmss

