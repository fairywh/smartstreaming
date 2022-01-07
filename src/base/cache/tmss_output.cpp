/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */
#include <fcntl.h>
#include <unistd.h>
#include "tmss_output.hpp"
#include <util/timer.hpp>
#include "tmss_channel.hpp"
#include <log/log.hpp>
#include <util/util.hpp>


namespace tmss {
const int max_output_queue_size = 1000;
int write_packet(void *opaque, uint8_t *buf, int buf_size) {
    OutputHandler* output = static_cast<OutputHandler*>(opaque);
    return output->write_msg(reinterpret_cast<char*>(buf), buf_size);
}

OutputHandler::OutputHandler(std::shared_ptr<Pool<OutputHandler>> pool,
            std::shared_ptr<Channel> channel) :
        PacketQueue(max_output_queue_size), ICoroutineHandler("output"),
        output_pool(pool) {
    is_stop = false;
    status = EOutputInit;
    this->channel = channel;
}

OutputHandler::~OutputHandler() {
    tmss_info("~output_handler");
}

int OutputHandler::write_msg(char* buff, int size) {
    tmss_info("write data, connid={}, {}, {}", output_conn->get_id(), size, PrintBuffer(buff, size));
    //  return output_conn->write(buff, size);
    return client->write_data(buff, size);
}

void OutputHandler::init_conn(std::shared_ptr<IClientConn> conn) {
    output_conn = conn;
}

void OutputHandler::init_format(std::shared_ptr<IMux> mux) {
    this->mux = mux;
}

void OutputHandler::init_play_client(std::shared_ptr<IClient> play_client) {
    this->client = play_client;
}

void OutputHandler::set_forward_address(Address& forward_address) {
    this->forward_address = forward_address;
}

void OutputHandler::set_forward_url(const std::string& forward_url) {
    this->forward_url = forward_url;
}

std::shared_ptr<IContext> OutputHandler::get_context() {
    return context;
}

void OutputHandler::set_context(std::shared_ptr<IContext> ctx) {
    context = ctx;
}

EOutputType OutputHandler::get_type() {
    return output_type;
}

void OutputHandler::set_type(EOutputType type) {
    output_type = type;
}

int OutputHandler::init_output(std::shared_ptr<IContext> input_context) {
    int out_buf_size = 1024 * 16;
    uint8_t* out_buf = new uint8_t[out_buf_size];

    return mux->init_output(out_buf, out_buf_size,
        this, write_packet,
        static_cast<void*>(input_context.get()), static_cast<void*>(context.get()));
}

int OutputHandler::run() {
    // tmss_info("output_conn ref_count={}", output_conn.use_count());
    output_pool->add(std::dynamic_pointer_cast<OutputHandler>(shared_from_this()));
    tmss_info("output start");
    return start();
}

int OutputHandler::set_stop() {
    int ret = error_success;
    is_stop = true;

    return ret;
}

int OutputHandler::cycle() {
    tmss_info("output running");
    int ret = error_success;
    if (output_type == EOutputForawrd) {
        if (status == EOutputInit) {
            int ret = output_conn->connect(forward_address);
            if (ret != 0) {
                tmss_error("connect forward error, {}", ret);
                return ret;
            }

            ret = mux->forward(forward_url, output_conn);
            if (ret != 0) {
                tmss_error("request forward error, {}", ret);
                return ret;
            }
            status = EOutputStart;
        } else {
        }
    } else {
    // play
        status = EOutputStart;
    }

    start_at = get_cache_time();
    last_send_at = get_cache_time();

    while (true) {
        if (is_stop) {
            tmss_info("output stop");
            break;
        }
        if (output_conn->is_stop()) {
            tmss_info("output conn stop");
            break;
        }
        std::shared_ptr<IPacket> packet;
        ret = dequeue(packet, 2 * 1000 * 1000);
        if (ret != error_success) {
            // to do
            /*if (get_cache_time() - last_send_at > 10 * 1000) {
                // timeout
                mux->send_status(404);
                break;
            }
            continue;//*/
            tmss_info("dequeue error,{}", ret);
            break;
        }
        // check timeout
        if (packet) {
            tmss_info("get the packet, size={}", packet->get_size());
            mux->send_status(200);
            ret = mux->handle_output(packet);
            if (ret != error_success) {
                tmss_info("send packet to output failed, {}", ret);
                break;
            }
        } else {
            tmss_info("send 404");
            mux->send_status(404);
            break;
        }
        last_send_at = get_cache_time();
    }

    return ret;
}

int OutputHandler::on_thread_stop() {
    int ret = error_success;

    status = EOutputInit;

    if (output_conn->is_stop()) {
        // connection error
    } else {
        output_conn->close();
        // tmss_info("output_conn is already stop");
        output_conn->set_stop();
    }
    // tmss_info("channel to delete output,channel_count={}", channel.use_count());
    if (channel.lock()) {
        // tmss_info("channel delete output,channel_count={}", channel.use_count());
        channel.lock()->del_output(std::dynamic_pointer_cast<OutputHandler>(shared_from_this()));
    }

    // tmss_info("output_conn ref_count={}", output_conn.use_count());
    output_pool->remove(std::dynamic_pointer_cast<OutputHandler>(shared_from_this()));
    tmss_info("output stop, conn_id={}", output_conn->get_id());

    return ret;
}

}  // namespace tmss

