/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/26.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#include <io/io_buffer.hpp>
#include <defs/err.hpp>
#include <log/log.hpp>


namespace tmss {
Buffer::Buffer() {
    buf = nullptr;
    size = 0;
    rpos = wpos = 0;
}

Buffer::Buffer(char* buffer, int size) {
    this->buf = buffer;
    this->size = size;
    rpos = wpos = 0;
}

Buffer::~Buffer() {
}

int Buffer::get_size() {
    return size;
}

int Buffer::write_left() {
    return (rpos + size - wpos - 1) % size;
}

int Buffer::read_left() {
    return (wpos + size - rpos) % size;
}

int Buffer::continuous_write_left() {
    if (wpos >= rpos) {
        return size - wpos;
    } else {
        return write_left();
    }
}

int Buffer::continuous_read_left() {
    if (rpos > wpos) {
        return size - rpos;
    } else {
        return read_left();
    }
}

char* Buffer::start() {
    return buf;
}

char* Buffer::wcurrent() {
    return buf + wpos;
}

char* Buffer::rcurrent() {
    return buf + rpos;
}

int Buffer::get_wpos() {
    return wpos;
}
int Buffer::get_rpos() {
    return rpos;
}

int Buffer::read_bytes(char* dst, int &len) {
    int ret = error_success;
    if (len <= 0) {
        ret = error_buffer_read;
        return ret;
    }
    if (len < read_left()) {
    } else {
        len = read_left();
    }

    int next_rpos = (rpos + len) % size;
    if (next_rpos < rpos) {
        memcpy(dst, buf + rpos, size - rpos);
        memcpy(dst + size - rpos, buf, next_rpos);
    } else {
        memcpy(dst, buf + rpos, len);
    }
    rpos = next_rpos;
    return ret;
}

int8_t Buffer::read_1byte() {
    // to do
    assert(read_left() >= 1);
    int8_t result = *(reinterpret_cast<int8_t*>(buf + rpos));
    seek_read(1);
    return result;
}

int16_t Buffer::read_2bytes() {
    assert(read_left() >= 2);

    int16_t value = 0;
    char* pp = reinterpret_cast<char*>(&value);
    pp[1] = *(buf + rpos);
    seek_read(1);
    pp[0] = *(buf + rpos);
    seek_read(1);

    return value;
}

int32_t Buffer::read_3bytes() {
    assert(read_left() >= 3);

    int32_t value = 0;
    char* pp = reinterpret_cast<char*>(&value);
    pp[2] = *(buf + rpos);
    seek_read(1);
    pp[1] = *(buf + rpos);
    seek_read(1);
    pp[0] = *(buf + rpos);
    seek_read(1);

    return value;
}

int32_t Buffer::read_4bytes() {
    assert(read_left() >= 4);

    int32_t value = 0;
    char* pp = reinterpret_cast<char*>(&value);
    pp[3] = *(buf + rpos);
    seek_read(1);
    pp[2] = *(buf + rpos);
    seek_read(1);
    pp[1] = *(buf + rpos);
    seek_read(1);
    pp[0] = *(buf + rpos);
    seek_read(1);

    return value;
}

int64_t Buffer::read_8bytes() {
    assert(read_left() >= 8);

    int64_t value = 0;
    char* pp = reinterpret_cast<char*>(&value);
    pp[7] = *(buf + rpos);
    seek_read(1);
    pp[6] = *(buf + rpos);
    seek_read(1);
    pp[5] = *(buf + rpos);
    seek_read(1);
    pp[4] = *(buf + rpos);
    seek_read(1);
    pp[3] = *(buf + rpos);
    seek_read(1);
    pp[2] = *(buf + rpos);
    seek_read(1);
    pp[1] = *(buf + rpos);
    seek_read(1);
    pp[0] = *(buf + rpos);
    seek_read(1);

    return value;
}

std::string Buffer::read_string(int len) {
    assert(read_left() >= len);

    std::string result;
    int next_rpos = (rpos + len) % size;
    if (next_rpos < rpos) {
        result.append(buf + rpos, size - rpos);
        result.append(buf, next_rpos);
    } else {
        result.append(buf + rpos, len);
    }
    seek_read(len);
    return result;
}

int Buffer::write_bytes(const char *src, int len) {
    int ret = error_success;
    if (len <= 0) {
        ret = error_buffer_read;
        return ret;
    }
    if (len > write_left()) {
        ret = error_buffer_read;
        return ret;
    }

    int next_wpos = (wpos + len) % size;
    if (next_wpos < wpos) {
        memcpy(buf + wpos, src, size - wpos);
        memcpy(buf, src + (size - wpos), next_wpos);
    } else {
        memcpy(buf + wpos, src, len);
    }

    wpos = next_wpos;
    return ret;
}

int Buffer::write_1byte(int8_t value) {
    // to do
    assert(write_left() >= 1);

    int ret = error_success;

    *(buf + wpos) = value;
    seek_write(1);
    return ret;
}

int Buffer::write_2bytes(int16_t value) {
    assert(write_left() >= 2);

    int ret = error_success;

    char* pp = reinterpret_cast<char*>(&value);
    *(buf + wpos) = pp[1];
    seek_write(1);
    *(buf + wpos) = pp[0];
    seek_write(1);
    return ret;
}

int Buffer::write_3bytes(int32_t value) {
    assert(write_left() >= 3);

    int ret = error_success;

    char* pp = reinterpret_cast<char*>(&value);
    *(buf + wpos) = pp[2];
    seek_write(1);
    *(buf + wpos) = pp[1];
    seek_write(1);
    *(buf + wpos) = pp[0];
    seek_write(1);
    return ret;
}

int Buffer::write_4bytes(int32_t value) {
    assert(write_left() >= 4);

    int ret = error_success;

    char* pp = reinterpret_cast<char*>(&value);
    *(buf + wpos) = pp[3];
    seek_write(1);
    *(buf + wpos) = pp[2];
    seek_write(1);
    *(buf + wpos) = pp[1];
    seek_write(1);
    *(buf + wpos) = pp[0];
    seek_write(1);
    return ret;
}

int Buffer::write_8bytes(int64_t value) {
    assert(write_left() >= 8);

    int ret = error_success;

    char* pp = reinterpret_cast<char*>(&value);
    *(buf + wpos) = pp[7];
    seek_write(1);
    *(buf + wpos) = pp[6];
    seek_write(1);
    *(buf + wpos) = pp[5];
    seek_write(1);
    *(buf + wpos) = pp[4];
    seek_write(1);
    *(buf + wpos) = pp[3];
    seek_write(1);
    *(buf + wpos) = pp[2];
    seek_write(1);
    *(buf + wpos) = pp[1];
    seek_write(1);
    *(buf + wpos) = pp[0];
    seek_write(1);
    return ret;
}

int Buffer::seek_read(int len) {
    int ret = error_success;
    if (len <= 0) {
        ret = error_buffer_read;
        return ret;
    }
    if (len < read_left()) {
    } else {
        //  len = read_left();
        ret = error_buffer_read;
        return ret;
    }

    int next_rpos = (rpos + len) % size;
    rpos = next_rpos;
    return ret;
}

int Buffer::seek_write(int len) {
    int ret = error_success;
    if (len <= 0) {
        ret = error_buffer_read;
        return ret;
    }
    if (len < write_left()) {
    } else {
        //  len = write_left();
        ret = error_buffer_read;
        return ret;
    }

    int next_wpos = (wpos + len) % size;

    wpos = next_wpos;
    return ret;
}

void Buffer::reset() {
    rpos = wpos = 0;
}

bool Buffer::read_require(int required_size) {
    //  srs_assert(required_size >= 0);
    if (required_size < 0) {
        return false;
    }
    return required_size <= read_left();
}

bool Buffer::read_empty() {
    return read_left() == 0;
}

IOBuffer::IOBuffer(std::shared_ptr<IReaderWriter> reader, std::shared_ptr<Buffer> buffer) {
    this->reader = reader;
    this->buffer = buffer;
    no_cache = false;
}

int IOBuffer::read_bytes(char* dst, int len) {
    int read_size = 0;
    // if the buffer has data already, it shoulde be red
    int wanted_size = len;
    read_from_cache(dst, wanted_size);
    read_size += wanted_size;

    // read left data
    if (no_cache) {
        // for no copy
        // read left directly from connection
        int read_from_reader = reader->read(dst + read_size, len - read_size);
        read_size += read_from_reader;
        tmss_info("read from connection, read_size={}", read_size);
    } else {
        int need_read_size = len - read_size;
        if (need_read_size == 0) {
            tmss_info("enough, read_size={}", read_size);
            return read_size;
        }

        // read to buffer
        int continuous_write_left_size = buffer->continuous_write_left();
        int read_size_from_reader = reader->read(buffer->wcurrent(), continuous_write_left_size);
        if (read_size_from_reader < 0) {
            tmss_error("read from connection error,ret={}", read_size_from_reader);
            return read_size_from_reader;
        }
        buffer->seek_write(read_size_from_reader);

        wanted_size = need_read_size;
        read_from_cache(dst + read_size, wanted_size);
        read_size += wanted_size;

        tmss_info("write to cache and read, write={},need_read={}, once_read={}, read={}",
            read_size_from_reader, need_read_size, wanted_size, read_size);
    }
    tmss_info("read_size={}", read_size);
    return read_size;
}

int IOBuffer::write_bytes(const char *src, int size) {
    return 0;
}

int IOBuffer::seek_read(int len) {
    return buffer->seek_read(len);
}

int IOBuffer::seek_write(int len) {
    return buffer->seek_write(len);
}

int IOBuffer::read_from_cache(char* dst, int& len) {
    int ret = error_success;
    int read_size = 0;
    int cache_size = buffer->read_left();
    if (cache_size > 0) {
        if (cache_size >= len) {
            buffer->read_bytes(dst, len);
            tmss_info("read from cache, len={}, cache_size={}",
                len, cache_size);
            // len is not changed
            return ret;
        } else {
            buffer->read_bytes(dst, cache_size);
            read_size += cache_size;
        }
    }
    len = read_size;
    tmss_info("read from cache, len={}, cache_size={}",
                len, cache_size);
    return ret;
}

}  // namespace tmss
