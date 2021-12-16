/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#pragma once
#include <string>
#include <iostream>
#include "tmss_cache.hpp"
#include <net/tmss_conn.hpp>
#include <format/mux.hpp>
#include <format/demux.hpp>
#include <net/parser.hpp>
#include <protocol/client.hpp>


namespace tmss {
class File {
    friend class FileInputHandler;
 private:
    std::string key;    // unique
    int         size;   // buffer size
    char*       data;
    int       pos;    // wrtie tail
    std::string name;   // file name in url
    int         total_length;    // read from content length
    // std::shared_ptr<FileInputHandler> input_handler;

    st_cond_t cond;
    int64_t  update_time_ms;

 public:
    File();
    explicit File(const std::string & file_name);
    virtual ~File();

    std::shared_ptr<File> copy();
    bool complete();
    int left_size();

    int init_buffer(int new_size);
    /*
    *   write new data to file cache
    */
    int append(std::shared_ptr<IPacket> packet);

    int append(const char* buffer, int append_size);
    /*
    *   get data from file cache
    */
    // int seek_range(std::vector<std::shared_ptr<IPacket>> packet_list,
    //    int offset = 0, int range = -1);

    /*
    *   get data from file cache
    */
    int seek_range(char* buffer, int& wanted_size,
        int offset = 0, int timeout_us = -1);

    // int add_input(std::shared_ptr<FileInputHandler> input);
    // int del_input(std::shared_ptr<FileInputHandler> input);

    std::string get_name() { return name; }
    int get_total_length()  { return total_length; }
    int get_current_length() { return pos; }

    int64_t get_update_time()   { return update_time_ms; }
};

class FileCache {
 public:
    FileCache() = default;
    virtual ~FileCache() = default;

    int add_file(const std::string& name, std::shared_ptr<File> file);
    std::shared_ptr<File> get_file(const std::string& file_name);
    int del_file(const std::string& file_name);
    void clear();

 private:
    std::map<std::string, std::shared_ptr<File>> file_cache;
};

class FileInputHandler : public ICoroutineHandler {
 public:
    FileInputHandler(std::shared_ptr<File> file, std::shared_ptr<Pool<FileInputHandler>> pool);
    virtual ~FileInputHandler() = default;

    int fetch_stream(char* buff, int &wanted_size);
    void init_conn(std::shared_ptr<IClientConn> conn);
    void init_format(std::shared_ptr<IDeMux> demux);
    void set_origin_address(Address& origin_address);
    void set_origin_info(Address& origin_address,
        const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& ext,
        const std::string& params);

    std::shared_ptr<IContext> get_context();
    void set_context(std::shared_ptr<IContext> ctx);
    EInputType get_type();
    void set_type(EInputType type);

    int init_input();

    int run();
    // int stop();

    // bool check_expire();

    int cycle() override;
    int on_thread_stop() override;

 private:
    std::string file_name;
    Address     origin_address;
    Request origin_request;
    EInputType         input_type;
    std::shared_ptr<IClientConn> input_conn;
    std::shared_ptr<IClient> client;
    std::shared_ptr<IDeMux> demux;
    std::shared_ptr<File> file;

    std::shared_ptr<IContext>          context;

    EStatusSource status;

    int total_size;     // when it is a static file, it is the file size
    int handle_size;    // the size already receive

    std::shared_ptr<Pool<FileInputHandler>> pool;
};

}  // namespace tmss


