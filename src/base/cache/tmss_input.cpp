/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#include "tmss_input.hpp"
#include "tmss_channel.hpp"
#include <log/log.hpp>
#include <util/util.hpp>

namespace tmss {
const int max_input_queue_size = 1000;

int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    InputHandler* input = static_cast<InputHandler*>(opaque);
    return input->fetch_stream(reinterpret_cast<char*>(buf), buf_size);   // client_conn
}

InputHandler::InputHandler(std::shared_ptr<Pool<InputHandler>> pool,
            std::shared_ptr<Channel> channel) :
        PacketQueue(max_input_queue_size), ICoroutineHandler("input"),
        input_pool(pool) {
    is_stop = false;
    status = ESourceInit;
    this->channel = channel;

    origin_try_count = 1;
}

InputHandler::~InputHandler() {
    tmss_info("~input_handler");
}

int InputHandler::set_connection(std::shared_ptr<IClientConn> conn) {
    if (!conn) {
        return -1;
    }
    input_conn = conn;

    return 0;
}

bool InputHandler::can_use() {
    return !is_stop;
}

int InputHandler::origin_start() {
    int count = 0;
    int ret = error_success;
    while (count < origin_try_count) {
        if (count > 0) {
            st_usleep(1 * 1000 * 1000);
        } else {
        }
        count++;
        ret = input_conn->connect(origin_address);
        if (ret != 0) {
            tmss_error("connect origin error,{}", origin_address.str());
            continue;
        }
        if (!client) {
            tmss_error("client is null");
            ret = error_ingest_no_client;
            return ret;
        }
        ret = client->request(origin_request.vhost,
            origin_request.path,
            origin_request.name,
            origin_request.params,
            demux);
        /*ret = demux->ingest(origin_request.vhost,
            origin_request.path,
            origin_request.name,
            origin_request.params,
            input_conn);    //  */
        if (ret != 0) {
            tmss_error("ingest origin error,{},{}", ret, origin_request.to_str());
            continue;
        }

        tmss_info("ingest origin success,{}", origin_request.to_str());
    }

    return ret;
}

int InputHandler::fetch_stream(char* buff, int wanted_size) {
    // int read_size = input_conn->read(buff, wanted_size);
    /*int read_size = io_buffer->read_bytes(buff, wanted_size);
    if (read_size < 0) {
        tmss_error("fetch stream failed, ret={}", read_size);
        return read_size;
    }
    tmss_info("read data, {}, {}", read_size, PrintBuffer(buff, read_size));
    return read_size;   //  */
    return client->read_data(buff, wanted_size);
}

void InputHandler::init_conn(std::shared_ptr<IClientConn> conn) {
    input_conn = conn;

    /*int in_buf_size = 1024 * 16;
    char* in_buf = new char[in_buf_size];
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(in_buf, in_buf_size);
    io_buffer = std::make_shared<IOBuffer>(conn, buffer);   //  */
}

void InputHandler::init_format(std::shared_ptr<IDeMux> demux) {
    this->demux = demux;
}

void InputHandler::init_origin_client(std::shared_ptr<IClient> origin_client) {
    this->client = origin_client;
}

void InputHandler::set_origin_address(Address& origin_address) {
    this->origin_address = origin_address;
}

void InputHandler::set_origin_info(Address& origin_address,
        const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& ext,
        const std::string& params) {
    this->origin_address = origin_address;
    this->origin_request.vhost = origin_host;
    this->origin_request.path = origin_path;
    this->origin_request.name = stream;
    this->origin_request.name += ".";
    this->origin_request.name += ext;
    this->origin_request.params = params;
}

std::shared_ptr<IContext> InputHandler::get_context() {
    return context;
}

void InputHandler::set_context(std::shared_ptr<IContext> ctx) {
    context = ctx;
}

EInputType InputHandler::get_type() {
    return input_type;
}

void InputHandler::set_type(EInputType type) {
    input_type = type;
}

int InputHandler::init_input() {
    int in_buf_size = 1024 * 8;
    uint8_t* in_buf = new uint8_t[in_buf_size];
    return demux->init_input(in_buf, in_buf_size,
        this, read_packet, static_cast<void*>(context.get()));
}

int InputHandler::run() {
    input_pool->add(std::dynamic_pointer_cast<InputHandler>(shared_from_this()));
    tmss_info("input start");
    return start();
}

int InputHandler::set_stop() {
    int ret = error_success;
    is_stop = true;

    return ret;
}

int InputHandler::cycle() {
    tmss_info("input running");
    int ret = error_success;
    for (int cnt = 0; cnt < 2; cnt++) {
        ret = cycle_interleave();
        if (ret != error_success) {
            tmss_error("cycle error, {}", ret);
            continue;
        }
    }

    return ret;
}

int InputHandler::on_thread_stop() {
    send_no_msg();

    int ret = error_success;

    status = ESourceInit;

    if (input_conn->is_stop()) {
    } else {
        input_conn->close();
        // tmss_info("input_conn is already stop");
        input_conn->set_stop();
    }
    if (channel.lock()) {
        channel.lock()->del_input(std::dynamic_pointer_cast<InputHandler>(shared_from_this()));
    }
    input_pool->remove(std::dynamic_pointer_cast<InputHandler>(shared_from_this()));

    tmss_info("channel to delete input,channel_count={}", channel.use_count());

    return ret;
}

int InputHandler::probe() {
    int ret = error_success;
    if (input_type == EInputOrigin) {
        if (status == ESourceInit) {
            ret = origin_start();
            if (ret != error_success) {
                tmss_error("origin start error, {}", ret);
                return ret;
            }
            status = ESourceStart;
        } else {
        }
    } else {
    // publish
        status = ESourceStart;
    }
    client->io_buffer->set_no_cache();
    return ret;
}

int InputHandler::cycle_interleave() {
    int ret = error_success;
    ret = probe();
    if (ret != error_success) {
        tmss_error("probe error, {}", ret);
        return ret;
    }
    while (true) {
        if (is_stop) {
            tmss_info("input stop");
            break;
        }
        if (input_conn->is_stop()) {
            tmss_info("input conn stop");
            break;
        }
        std::shared_ptr<IPacket> packet;
        ret = demux->handle_input(packet);
        if (ret != error_success) {
            tmss_info("get packet from input failed, {}", ret);
            break;
        }
        tmss_info("get a new packet");
        enqueue(packet);
    }
    return ret;
}

}  // namespace tmss

