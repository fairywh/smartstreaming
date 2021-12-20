/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/10.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#include "co_threads.hpp"

#include <pthread.h>
#include <st.h>

#include <utility>

//  #include "log/log.hpp"
#include "defs/err.hpp"

StThreadName::StThreadName(const char *name) {
    pthread_setname_np(pthread_self(), name);
}

void ICoThreadHandler::on_start() {
}

void ICoThreadHandler::on_stop() {
}

void ICoThreadHandler::on_destroy() {
}

CoThread::CoThread(std::string thread_name,
        std::shared_ptr<ICoThreadHandler> h): name(std::move(thread_name)) {
    handler       = std::move(h);
    tid           = 0;
}

CoThread::~CoThread() {
    if (handler) {
        handler->on_destroy();
    }
}

int CoThread::run() {
    int status;
    if ((status = pthread_create(&tid, nullptr, thread_start, static_cast<void*>(this)))!= 0) {
        //  sp_error_f("srs st run failed status={}", status);
        return status;
    }
    return error_success;
}

void* CoThread::thread_start(void *co_thread) {
    auto* self = static_cast<CoThread*>(co_thread);
    StThreadName tn(self->name.c_str());

    if (st_init() != 0) {
        //  sp_error("st init failed");
        return nullptr;
    }
    self->handler->on_start();
    self->handler->run();
    self->handler->on_stop();
    return nullptr;
}

