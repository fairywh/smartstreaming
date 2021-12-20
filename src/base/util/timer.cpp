/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/24.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#include <util/timer.hpp>
#include <sys/time.h>
#include <ctime>
//  #include "log/log.hpp"

namespace tmss {
static utime_t global_cache_time = 0;

utime_t get_cache_time() {
    if (global_cache_time == 0) {
        return update_time();
    }
    return global_cache_time;
}

utime_t update_time() {
    timeval now{};

    if (gettimeofday(&now, nullptr) < 0) {
        //  tmss_error("gettimeofday failed, ignore");
        return 0;
    }

    utime_t now_us = ((int64_t)now.tv_sec) * 1000 * 1000 + (int64_t) now.tv_usec;

    global_cache_time = now_us;
    return now_us;
}

StTimer::StTimer(utime_t v) : ICoroutineHandler("st-timer") {
    // co       = std::make_shared<STCoroutine>("st-timer", this, get_ctx_id());
    interval = v;
}

error_t StTimer::run() {
    return start();
}

error_t StTimer::cycle() {
    while (true) {
        update_time();
        st_usleep(interval);
    }
    return error_success;
}

MonitorTimer::MonitorTimer(utime_t t) {
    interval    = t;
    last_report = 0;
}

bool MonitorTimer::next() {
    if (get_cache_time() - last_report >= interval) {
        last_report = get_cache_time();
        return true;
    }
    return false;
}
#if 0
StatTimer * StatTimer::get_stat_timer() {
    thread_local StatTimer* instance = nullptr;

    if (instance == nullptr) {
        instance = new StatTimer();
        instance->run();
    }
    return instance;
}

StatTimer::StatTimer() {
    co    = std::make_shared<STCoroutine>("stat-timer", this, get_ctx_id());
    perf  = std::make_shared<SRT_TRACEBSTATS>();
    *perf = {0};
}

error_t StatTimer::cycle() {
    while (true) {
        if (true) {
            std::string tmp;
            st_srt_stat_print(tmp, perf.get());

            // 重传率
            double re_send = st_srt_stat_retrans(perf.get(), true);
            double re_rcv  = st_srt_stat_retrans(perf.get(), false);

            // 丢包率
            double send_loss = st_srt_stat_loss(perf.get(), true);
            double recv_loss = st_srt_stat_loss(perf.get(), false);

            //  tmss_info("total srt trace:{}", tmp);

            const int MB = 1024 * 1024;

            *perf = {0};
        }
        st_sleep(60);
    }
}

error_t StatTimer::run() {
    return co->start();
}
#if 0
error_t StatTimer::stat(SRT_TRACEBSTATS *f) {
    perf->byteSentUnique      += f->byteSentUnique;
    perf->byteRecvUnique      += f->byteRecvUnique;

    perf->byteSentUniqueTotal += f->byteSentUniqueTotal;
    perf->byteRecvUniqueTotal += f->byteRecvUniqueTotal;

    perf->byteSent            += f->byteSent;
    perf->byteRecv            += f->byteRecv;

    perf->byteSndDropTotal    += f->byteSndDropTotal;
    perf->byteRcvDropTotal    += f->byteRcvDropTotal;

    perf->byteRcvDrop         += f->byteRcvDrop;
    perf->byteSndDrop         += f->byteSndDrop;

    return error_success;
}
#endif
#endif

}   // namespace tmss

