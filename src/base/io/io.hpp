/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng(邓伟). io读写操作
 *
 * =====================================================================================
 */
#pragma once

#include <sys/uio.h>

namespace tmss {

class IReader {
 public:
    virtual ~IReader() = default;
    virtual int read(char* buf, int size) = 0;
    virtual int readv(const iovec *iov, int iov_size) { }
};

class IWriter {
 public:
    virtual ~IWriter() = default;
    virtual int write(const char* buf, int size) = 0;
    virtual int writev(const iovec *iov, int iov_size) { }
};

class IReaderWriter : public IReader, public IWriter {
};

}  // namespace tmss
