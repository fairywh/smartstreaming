/* Copyright [2019] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/11.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#pragma once
#include <sys/uio.h>
#include "defs/err.hpp"

class IReader {
 public:
    virtual ~IReader() = default;

 public:
    virtual void    set_recv_timeout(utime_t tm) = 0;
    virtual utime_t get_recv_timeout() = 0;

 public:
    /**
     * 读满数据
     */
    virtual error_t read_fully(void* buf, size_t size, ssize_t* nread) = 0;

    /**
     * 尽量读数据
     */
    virtual error_t read(void* buf, size_t size, size_t& nread) = 0;
};

class IWriter {
 public:
    virtual ~IWriter() = default;

 public:
    virtual void set_send_timeout(utime_t tm) = 0;
    virtual utime_t get_send_timeout() = 0;

 public:
    virtual error_t write(void* buf, size_t size) = 0;
};

class IReaderWriter : virtual public IReader, virtual public IWriter {
 public:
    IReaderWriter() = default;
    ~IReaderWriter() override = default;
};

