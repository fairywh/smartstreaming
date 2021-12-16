/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/10.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */

#pragma once

#include <pthread.h>
#include <st.h>
#include <string>
#include <memory>

#include "defs/err.hpp"

extern int get_ctx_id();

class STCoroutine;
class ICoroutineHandler : public std::enable_shared_from_this<ICoroutineHandler> {
 public:
    explicit ICoroutineHandler(const std::string name);
    virtual ~ICoroutineHandler() = default;

 public:
    virtual int start();
    virtual error_t cycle() = 0;
    virtual void stop();

    virtual error_t on_thread_stop();

 protected:
    std::shared_ptr<STCoroutine> co;
    std::string coroutine_name;
    bool    running;
};

// The corotine object.
class ICoroutine {
 public:
    ICoroutine() = default;
    virtual ~ICoroutine() = default;

 public:
    virtual error_t start() = 0;
    virtual void stop() = 0;
    virtual void interrupt() = 0;
    // @return a copy of error, which should be freed by user.
    //      NULL if not terminated and user should pull again.
    virtual error_t pull() = 0;
    virtual int cid() = 0;
};

// For utest to mock the thread create.
typedef void* (*_ST_THREAD_CREATE_PFN)(void *(*start)(void *arg),
        void *arg, int joinable, int stack_size);

extern _ST_THREAD_CREATE_PFN _pfn_st_thread_create;

class STCoroutine : public ICoroutine {
 private:
    std::string name;
    std::weak_ptr<ICoroutineHandler> handler;

 private:
    st_thread_t trd;
    int context;
    error_t trd_err;
    st_cond_t cond;

 private:
    bool started;
    bool interrupted;
    bool disposed;
    // Cycle done, no need to interrupt it.
    bool cycle_done;

 public:
    // Create a thread with name n and handler h.
    // @remark User can specify a cid for thread to use, or we will allocate a new one.
    STCoroutine(std::string n, std::shared_ptr<ICoroutineHandler> h, int cid = 0);
    ~STCoroutine() override;

 public:
    // Start the thread.
    // @remark Should never start it when stopped or terminated.
    error_t start() override;
    // Interrupt the thread then wait to terminated.
    // @remark If user want to notify thread to quit async, for example if there are
    //      many threads to stop like the encoder, use the interrupt to notify all threads
    //      to terminate then use stop to wait for each to terminate.
    void stop() override;
    // Interrupt the thread and notify it to terminate, it will be wakeup if it's blocked
    // in some IO operations, such as st_read or st_write, then it will found should quit,
    // finally the thread should terminated normally, user can use the stop to join it.
    void interrupt() override;
    // Check whether thread is terminated normally or error(stopped or termianted with error),
    // and the thread should be running if it return ERROR_SUCCESS.
    // @remark Return specified error when thread terminated normally with error.
    // @remark Return ERROR_THREAD_TERMINATED when thread terminated normally without error.
    // @remark Return ERROR_THREAD_INTERRUPED when thread is interrupted.
    error_t pull() override;
    // Get the context id of thread.
    int cid() override;

 private:
    virtual error_t cycle();
    static void* pfn(void* arg);
};

