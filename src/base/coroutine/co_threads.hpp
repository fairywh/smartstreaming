/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/10.
 *        Author:  weideng. 多线程st thread需要重新初始化,把多线程封装起来
 *
 * =====================================================================================
 */

#pragma once

#include <pthread.h>

#include <memory>
#include <string>

class StThreadName {
 public:
    explicit StThreadName(const char* name);
};

class ICoThreadHandler {
 public:
    virtual ~ICoThreadHandler() = default;

 public:
    virtual void run() = 0;

 public:
    virtual void on_start();
    virtual void on_stop();
    virtual void on_destroy();
};

class CoThread {
 public:
    CoThread(std::string thread_name, std::shared_ptr<ICoThreadHandler> handler);
    ~CoThread();

 public:
    int run();

 public:
    static void* thread_start(void *co_thread);

 private:
    std::shared_ptr<ICoThreadHandler> handler;
    std::string name;
    pthread_t tid;
};

