/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/26.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */
#pragma once
#include <memory>

#include <io.hpp>

namespace tmss {
/*
*   simple ring buffer
*/
class Buffer {
 public:
    Buffer();
    Buffer(char* buffer, int size);
    ~Buffer();

 public:
    int get_size();
    char* start();
    char* wcurrent();
    char* rcurrent();
    int get_wpos();
    int get_rpos();
    int read_bytes(char* dst, int &len);  // read to dst
    int8_t read_1byte();
    int16_t read_2bytes();
    int32_t read_3bytes();
    int32_t read_4bytes();
    int64_t read_8bytes();
    std::string read_string(int len);

    int write_bytes(const char *src, int len);    // write from src
    int write_1byte(int8_t value);
    int write_2bytes(int16_t value);
    int write_3bytes(int32_t value);
    int write_4bytes(int32_t value);
    int write_8bytes(int64_t value);

    int seek_read(int len);
    int seek_write(int len);

    int write_left();
    int read_left();

    int continuous_write_left();
    int continuous_read_left();

    void reset();

    bool read_require(int required_size);
    bool read_empty();

 private:
    char* buf;      // buffer begin
    int wpos;       // write pos
    int rpos;       // read pos
    int size;       // buffer size
};

class IOBuffer {
 public:
    IOBuffer(std::shared_ptr<IReaderWriter> reader, std::shared_ptr<Buffer> buffer);

 public:
    /*
    *   return, read_size
    */
    int read_bytes(char* dst, int len);  // read to dst

    int write_bytes(const char *src, int size);    // write from src

    void set_no_cache() { no_cache = true; }

    int seek_read(int len);
    int seek_write(int len);

 private:
    std::shared_ptr<Buffer> buffer;
    std::shared_ptr<IReader> reader;

    bool no_cache;  // to do

    int read_from_cache(char* dst, int& len);
};


};  // namespace tmss
