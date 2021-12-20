/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2020/9/24.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#pragma once

#include <memory>
#include "defs/err.hpp"
#include "coroutine/coroutine.hpp"
// #include "stnet/st_srt.hpp"
namespace tmss {
#define MONITOR_EVERY_MIN 60 * 1000 * 1000

utime_t get_cache_time();
utime_t update_time();


class StTimer : public ICoroutineHandler {
 public:
    explicit StTimer(utime_t interval = 1 * 1000);

 public:
    error_t cycle() override;

 public:
    error_t run();

 private:
    // std::shared_ptr<STCoroutine> co;
    utime_t interval;
};

class MonitorTimer {
 public:
    explicit MonitorTimer(utime_t t = MONITOR_EVERY_MIN);

 public:
    bool next();

 private:
    utime_t interval;
    utime_t last_report;
};
#if 0
class StatTimer : public ICoroutineHandler {
 public:
    static StatTimer* get_stat_timer();

 public:
    StatTimer();

 public:
    error_t cycle() override;

 public:
    error_t run();

 public:
    // error_t stat(SRT_TRACEBSTATS* f);

 private:
    // std::shared_ptr<SRT_TRACEBSTATS> perf;
    std::shared_ptr<STCoroutine> co;
    utime_t interval;
};
#endif


}   // namespace tmss

