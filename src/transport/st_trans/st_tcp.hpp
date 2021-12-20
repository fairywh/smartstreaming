/* Copyright [2019] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/15.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#pragma once

#include <st.h>

#include <string>

#include "defs/err.hpp"
#include "st_io.hpp"

/**
 * @param fd where the fd has accepted or opened
 * @return get st fd
 * 谨慎使用，跨线程传递文件句柄fd时，新线程必须重新打开fd,不重新设计flag
 */
st_netfd_t   st_open_socket_only(int fd);

/**
 * @param fd st fd mem free but the unix fd not close
 * 谨慎使用，跨线程传递文件句柄时，老线程需要销毁本线程的st对象，文件句柄需要在其他线程释放，否则造成泄露
 */
void         st_free_socket_only(st_netfd_t fd);

/**
 * dns 解析域名
 */
std::string dns_resolve(const std::string& host);

/**
 * st相关tcp的函数封装
 */
int           st_tcp_fd(st_netfd_t fd);
// 创建stfd，eg.用于tcp客户端上
error_t       st_tcp_create_fd(st_netfd_t& fd);
// 手动创建文件句柄fd，设置属性，需要生成对应的st句柄， eg. 主要用于服务器上
st_netfd_t    st_tcp_open_fd(int fd);
// 释放stfd，同时关闭文件句柄
error_t       st_tcp_close(st_netfd_t& fd);

error_t    st_tcp_listen(const std::string& server, int port,
                         int back_log, st_netfd_t& fd, bool reuse_port = true);
error_t    st_tcp_connect(const std::string& server, int port,
                          utime_t tm, st_netfd_t* fd);
st_netfd_t st_tcp_accept(st_netfd_t stfd, struct sockaddr *addr,
                         int *addrlen, utime_t timeout);
error_t    st_tcp_read(st_netfd_t stfd,  void *buf,
                       size_t nbyte, utime_t timeout);
error_t    st_tcp_readv(st_netfd_t stfd, const struct iovec *iov, int iov_size,
                        st_utime_t timeout);

error_t    st_tcp_write(st_netfd_t stfd, const void *buf,
                        size_t nbyte, utime_t timeout);

error_t    st_tcp_writev(st_netfd_t stfd, const iovec *iov, int iov_size,
                         utime_t timeout);

error_t    st_tcp_fd_reuseaddr(int fd, int enable = 1);
error_t    st_tcp_fd_reuseport(int fd);


class StTcpSocket : public IReaderWriter {
 public:
    explicit StTcpSocket(st_netfd_t stfd);
    ~StTcpSocket() override;

 public:
    // reader
    void    set_recv_timeout(utime_t tm) override;
    utime_t get_recv_timeout() override;

    error_t read_fully(void* buf, size_t size, ssize_t* nread) override;
    error_t read(void* buf, size_t size, size_t& nread) override;

 public:
    // writer
    void set_send_timeout(utime_t tm) override;
    utime_t get_send_timeout() override;
    error_t write(void* buf, size_t size) override;

 private:
    utime_t rtm;
    utime_t stm;
    // The recv/send data in bytes
    int64_t rbytes;
    int64_t sbytes;
    // The underlayer st fd.
    st_netfd_t stfd;
};

