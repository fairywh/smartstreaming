/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */
#include "tmss_static.hpp"
#include <algorithm>
#include "defs/err.hpp"
#include <log/log.hpp>
#include <util/timer.hpp>
#include <util/util.hpp>


namespace tmss {
int file_read_packet(void *opaque, uint8_t *buf, int buf_size) {
    FileInputHandler* input = static_cast<FileInputHandler*>(opaque);
    return input->fetch_stream(reinterpret_cast<char*>(buf), buf_size);   // client_conn
}

File::File() {
    this->data = nullptr;
    size = pos = total_length = 0;
    cond = st_cond_new();
    update_time_ms = 0;
}

File::File(const std::string & file_name) {
    this->data = nullptr;
    size = pos = total_length = 0;
    name = file_name;
    cond = st_cond_new();
    update_time_ms = 0;
}

File::~File() {
    st_cond_destroy(cond);
}

std::shared_ptr<File> File::copy() {
    std::shared_ptr<File> file = std::make_shared<File>();

    file->data = new char[size];
    file->size = size;

    return file;
}
bool File::complete() {
    if (total_length > 0) {
        return pos >= total_length;
    } else {
        return false;
    }
}

int File::left_size() {
    return size - pos;
}

int File::init_buffer(int new_size) {
    int ret = error_success;
    tmss_info("reset buffer, old_size={}, new_size={}", size, new_size);
    if (new_size >= 1024 * 1024 * 1024) {
        tmss_error("reset error, ret={}", ret);
        return error_file_buffer_too_large;
    }
    if (this->data) {
        if (new_size < size) {
            tmss_error("reset error, ret={}", ret);
            return error_file_buffer_init_small;
        }
        char* new_data = new char[new_size + 1];
        size = new_size;
        memcpy(new_data, data, pos);    // copy data to new buffer
    } else {
        this->data = new char[new_size + 1];
        size = new_size;
    }
    return ret;
}

int File::append(std::shared_ptr<IPacket> packet) {
    return append(packet->buffer(), packet->get_size());
}

int File::append(const char* buffer, int append_size) {
    int ret = error_success;

    if (append_size > left_size()) {
        // return error_file_buffer_not_enough;
        init_buffer(16 * (Max(size, append_size) + 1));   // to do
        if (append_size > left_size()) {
            tmss_error("file too large, append_size={}, left_size={}", append_size, left_size());
            return error_file_buffer_not_enough;
        }
    }
    memcpy(data, buffer, append_size);
    pos += append_size;

    // if (total_length < pos) {
        // maybe not set by content-length
        // total_length = pos;
    // }
    st_cond_broadcast(cond);

    tmss_info("file append, size={},total_length={},current_length={}",
        append_size, total_length, get_current_length());

    return ret;
}

int File::seek_range(char* buffer, int& wanted_size,
        int offset, int timeout_us) {
    int ret = error_success;

    if (offset >= pos) {
        // no new data
        tmss_info("no new data, wait");
        st_cond_timedwait(cond, timeout_us);
    }
    tmss_info("wanted_size={}", wanted_size);
    if (wanted_size + offset > get_current_length()) {
        wanted_size = get_current_length() - offset;
        tmss_info("wanted_size={}", wanted_size);
    }
    // char * start = static_cast<char*>(this->data + offset);
    memcpy(buffer, this->data + offset, wanted_size);

    return ret;
}

int FileCache::add_file(const std::string& name, std::shared_ptr<File> file) {
    int ret = error_success;
    auto iter = file_cache.find(name);
    if (iter == file_cache.end()) {
        file_cache.insert(std::make_pair(name, file));
    } else {
        iter->second = file;    // update
    }
    return ret;
}

std::shared_ptr<File> FileCache::get_file(const std::string& file_name) {
    auto iter = file_cache.find(file_name);
    if (iter == file_cache.end()) {
        return nullptr;
    }
    return iter->second;
}

int FileCache::del_file(const std::string& file_name) {
    int ret = error_success;
    auto iter = file_cache.find(file_name);
    if (iter == file_cache.end()) {
        return ret;
    } else {
        file_cache.erase(iter);
    }
    return ret;
}

void FileCache::clear() {
    file_cache.clear();
}

FileInputHandler::FileInputHandler(std::shared_ptr<File> file,
        std::shared_ptr<Pool<FileInputHandler>> pool) : ICoroutineHandler("file_input") {
    this->file = file;
    this->pool = pool;
    status = ESourceInit;
}

int FileInputHandler::fetch_stream(char* buff, int &wanted_size) {
    int read_size = input_conn->read(buff, wanted_size);
    tmss_info("read data_size={}", read_size);
    return read_size;
}

void FileInputHandler::init_conn(std::shared_ptr<IClientConn> conn) {
    this->input_conn = conn;
}

void FileInputHandler::init_format(std::shared_ptr<IDeMux> demux) {
    this->demux = demux;
}

void FileInputHandler::set_origin_address(Address& origin_address) {
    this->origin_address = origin_address;
}

void FileInputHandler::set_origin_info(Address& origin_address,
        const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& ext,
        const std::string& params) {
    this->origin_address = origin_address;
    this->origin_request.vhost = origin_host;
    this->origin_request.path = origin_path;
    this->origin_request.name = stream;
    this->origin_request.params = params;
}

std::shared_ptr<IContext> FileInputHandler::get_context() {
    return context;
}

void FileInputHandler::set_context(std::shared_ptr<IContext> ctx) {
    context = ctx;
}

EInputType FileInputHandler::get_type() {
    return input_type;
}

void FileInputHandler::set_type(EInputType type) {
    input_type = type;
}

int FileInputHandler::init_input() {
    int in_buf_size = 1024 * 16;
    uint8_t* in_buf = new uint8_t[in_buf_size];
    return demux->init_input(in_buf, in_buf_size,
        this, file_read_packet, static_cast<void*>(context.get()));
}

int FileInputHandler::run() {
    pool->add(std::dynamic_pointer_cast<FileInputHandler>(shared_from_this()));
    return start();
}

int FileInputHandler::cycle() {
    int ret = error_success;
    tmss_info("file input start");
    if (input_type == EInputOrigin) {
        if (status == ESourceInit) {
            int ret = input_conn->connect(origin_address);
            if (ret != 0) {
                tmss_error("origin connect error, {}", ret);
                return ret;
            }
            ret = client->request(origin_request.vhost,
                origin_request.path,
                origin_request.name,
                origin_request.params,
                demux);
            if (ret != 0) {
                tmss_error("origin ingest error, {}", ret);
                return ret;
            }
            status = ESourceStart;

            total_size = demux->get_total_length();

            file->init_buffer(total_size);

            file->total_length = total_size;

            tmss_info("start receiving file, total_length={}", total_size);

            file->update_time_ms = get_cache_time() / 1000;     //  to do
        } else {
        }
    }

    while (true) {
        if (input_conn->is_stop()) {
            tmss_info("input conn stop");
            break;
        }
        std::shared_ptr<IPacket> packet;
        int ret = demux->handle_input(packet);
        if (ret != error_success) {
            tmss_info("get packet from input failed, {}", ret);
            // to do
            if (total_size == 0) {
                total_size = handle_size;   // set total size to current read size
                file->total_length = total_size;
            }
            break;
        }

        if (!packet) {
            tmss_error("no packet");
            break;
        }
        handle_size += packet->get_size();

        // to do
        file->append(packet);
        // if it is a short connection (like static file)
        // break;
        if ((total_size > 0) && (handle_size >= total_size)) {
            // complete
            tmss_info("file complete, file_size={}", total_size);
            break;
        }
        // if chunk end
    }
    return ret;
}

int FileInputHandler::on_thread_stop() {
    status = ESourceInit;
    pool->remove(std::dynamic_pointer_cast<FileInputHandler>(shared_from_this()));
    return error_success;
}
}  // namespace tmss


