/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng. 网络连接
 *
 * =====================================================================================
 */
#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <memory>
#include <io/io.hpp>
#include <coroutine.hpp>

namespace tmss {

enum CONN_TYPE {
    CONN_TCP,
    CONN_SRT
};

class Address {
 public:
    Address();
    Address(const std::string& ip, int port);
    ~Address();
    std::string get_ip() { return remote_ip;}
    int get_port() { return remote_port;}
    std::string str();

 private:
    std::string remote_ip;
    int remote_port;
};

class IConn : public IReaderWriter {
 public:
    // bool IsConnected() = 0;
    IConn();
    virtual ~IConn() = default;
    virtual int get_id() { return id; }
    virtual int64_t get_recv_bytes()    { return 0;}
    virtual int64_t get_send_bytes()   { return 0;}

 protected:
    int id;
};

// for server
class IClientConn;
class IServerConn : public IConn, public ICoroutineHandler {
 public:
    IServerConn();
    virtual ~IServerConn();
    virtual int listen(const std::string &ip, int port)  = 0;
    virtual std::shared_ptr<IClientConn> accept()  = 0;
    virtual int close() = 0;
    virtual int run();
    virtual int read(char* buf, int size) { return 0;}
    virtual int write(const char* buf, int size) { return 0;}

 private:
};

class IConnHandler;
// for a endpoint to endpoint session
class IClientConn : public IConn, public ICoroutineHandler {
 public:
    IClientConn();
    virtual ~IClientConn();
    // connect to address
    virtual int connect(Address address) = 0;
    virtual int close() = 0;
    virtual int run();
    virtual int read(char* buf, int size) { return 0;}
    virtual int read_fully(char* buf, int size) { return 0; }
    virtual int write(const char* buf, int size) { return 0;}
    virtual int write_fully(const char* buf, int size) { return 0; }
    virtual void set_stop();
    virtual bool is_stop();
    // to do
    virtual void set_handler(std::shared_ptr<IConnHandler> conn_handler);

    void set_connect_timeout(int32_t timeout_ms);
    void set_send_timeout(int32_t timeout_ms);
    void set_recv_timeout(int32_t timeout_ms);

 protected:
    bool stop;
    st_cond_t cond;
    std::weak_ptr<IConnHandler>  handler;

    int32_t     connect_timeout_ms;
    int32_t     send_timeout_ms;
    int32_t     recv_timeout_ms;
};

extern int conn_id;
}  // namespace tmss

