/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/10.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */

#include <st.h>

#include <cstdint>
#include <string>

#include "coroutine.hpp"
//  #include "log/log.hpp"

using std::string;

_ST_THREAD_CREATE_PFN _pfn_st_thread_create = (_ST_THREAD_CREATE_PFN)st_thread_create;

int get_ctx_id() {
    thread_local int cid = 0;

     cid++;
     if (cid < 0) {
         cid = 0;
     }

     return cid;
}

ICoroutineHandler::ICoroutineHandler(const std::string name) : coroutine_name(name) {
    running = false;
}

int ICoroutineHandler::start() {
    if (!running) {
        co = std::make_shared<STCoroutine>(coroutine_name, shared_from_this(), get_ctx_id());
        running = true;
    } else {
        return error_success;
    }
    return co->start();
}

void ICoroutineHandler::stop() {
    co->stop();
}

error_t ICoroutineHandler::on_thread_stop() {
    running = false;
    return error_success;
}


STCoroutine::STCoroutine(string n, std::shared_ptr<ICoroutineHandler> h, int cid) :
        name(std::move(n)) {
    handler = h;
    context = cid;
    trd     = nullptr;
    trd_err = error_success;
    started = interrupted = disposed = cycle_done = false;
    cond    = st_cond_new();
}

STCoroutine::~STCoroutine() {
    STCoroutine::stop();
    st_cond_destroy(cond);
    //  sp_trace_f("coroutine closed name:{}", name);
}

error_t STCoroutine::start() {
    error_t err = error_success;

    if (started || disposed) {
        if (disposed) {
            err = error_cothread_start;
        } else {
            err = error_cothread_start;
        }

        if (trd_err == error_success) {
            trd_err = err;
        }
        return err;
    }

    if ((trd = (st_thread_t)_pfn_st_thread_create(pfn, this, 0, 0)) == nullptr) {
        //  sp_error("create thread failed");
        err = error_cothread_start;
        trd_err = err;
        return err;
    }

    //  //  tmss_info("name: {}, start success coroutine trd:{}", name, reinterpret_cast<int64_t>(trd));
    started = true;

    return err;
}

void STCoroutine::stop() {
    if (disposed) {
        return;
    }
    disposed = true;

    interrupt();

    // When not started, the rd is NULL.
    if (trd) {
        //  void* res = nullptr;
        int r0 = st_cond_wait(cond);

        // st_thread_join bug not free stack
        if (r0 != 0) {
            //  tmss_error("st_cond_wait failed r0:{}", r0);
        } else {
            //  tmss_error("st_cond_wait success r0:{}", r0);
        }

        error_t err_res = error_success;
        if (err_res != error_success) {
            // When worker cycle done, the error has already been overrided,
            // so the trd_err should be equal to err_res.
            // assert(false);
            return;
        }
    }

    // If there's no error occur from worker, try to set to terminated error.
    if (trd_err == error_success && !cycle_done) {
        trd_err = error_cothread_stop;
    }
}

void STCoroutine::interrupt() {
    if (!started || interrupted || cycle_done) {
        return;
    }
    interrupted = true;

    if (trd_err == error_success) {
        trd_err = error_cothread_interrupt;
    }

    st_thread_interrupt((st_thread_t)trd);
}

error_t STCoroutine::pull() {
    return (trd_err);
}

int STCoroutine::cid() {
    return context;
}

error_t STCoroutine::cycle() {
    if (!handler.lock()) {
        return error_success;
    }
    error_t err = handler.lock()->cycle();

    // Set cycle done, no need to interrupt it.

    st_cond_broadcast(cond);

    disposed = true;
    cycle_done = true;
    if (!handler.lock()) {
        return err;
    }
    handler.lock()->on_thread_stop();

    //  tmss_info("name:{} thread stop success", name);

    return err;
}

void* STCoroutine::pfn(void* arg) {
    auto p = static_cast<STCoroutine*>(arg);

    error_t err = p->cycle();

    // Set the err for function pull to fetch it.
    // @see https://github.com/ossrs/srs/pull/1304#issuecomment-480484151
    if (err != error_success) {
        // It's ok to directly use it, because it's returned by st_thread_join.
        p->trd_err = err;
    }

    return nullptr;
}
