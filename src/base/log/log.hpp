/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/29.
 *        Author:  weideng.
 *
 * =====================================================================================
 */
#pragma once
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <string>
#include <sstream>

namespace tmss {

#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? \
    __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)

//  typedef int (*log_func) (const char* fmt, ...);

#ifndef sp_default_logger_name
#define sp_default_logger_name "sp_default_logger"
#endif

#ifndef sp_default_stat_logger_name
#define sp_default_stat_logger_name "sp_default_stat_logger"
#endif

#ifndef sp_default_srt_lib_logger_name
#define sp_default_srt_lib_logger_name "sp_default_srt_lib_logger_name"
#endif

#ifndef one_mb
#define one_mb 1048576
#endif

/*enum level_enum {
    trace = SPDLOG_LEVEL_TRACE,
    debug = SPDLOG_LEVEL_DEBUG,
    info = SPDLOG_LEVEL_INFO,
    warn = SPDLOG_LEVEL_WARN,
    err = SPDLOG_LEVEL_ERROR,
    critical = SPDLOG_LEVEL_CRITICAL,
    off = SPDLOG_LEVEL_OFF,
};//*/

class Logger {
 public:
    Logger() = default;
    virtual ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&)= delete;

 private:
    std::shared_ptr<spdlog::logger> _sp_default_logger;
    std::shared_ptr<spdlog::logger> _sp_stat_logger;
    //  std::shared_ptr<spdlog::logger> _srt_lib_logger;

 public:
    static Logger* Instance() {
        static Logger instance;
        return &instance;
    }

    void set_logger(const std::string& file_name, int max_file_size_mb,
            int max_files, spdlog::level::level_enum l = spdlog::level::trace) {
        _sp_default_logger = add_logger(sp_default_logger_name,
                file_name, max_file_size_mb, max_files);
        _sp_default_logger->flush_on(spdlog::level::err);
        spdlog::flush_every(std::chrono::seconds(2));
        _sp_default_logger->set_level(l);
    }

    void set_stat_logger(const std::string& file_name, int max_file_size_mb, int max_files) {
        _sp_stat_logger = add_logger(sp_default_stat_logger_name,
                file_name, max_file_size_mb, max_files);
    }

    std::shared_ptr<spdlog::logger> add_logger(const std::string& logger_name,
            const std::string& file_name, int max_file_size_mb, int max_files) {
        return spdlog::rotating_logger_mt(logger_name, file_name,
                one_mb * max_file_size_mb, max_files);
    }

    std::shared_ptr<spdlog::logger> get_logger(const std::string& name) {
        return spdlog::get(name);
    }

    std::shared_ptr<spdlog::logger> get_logger() {
        if (this->_sp_default_logger == nullptr) {
            return get_console_logger();
        }
        return this->_sp_default_logger;
    }

    static std::shared_ptr<spdlog::logger> get_console_logger() {
        static auto console = spdlog::stdout_color_mt("console");
        return console;
    }
};

//  log_func register_logger;
extern spdlog::level::level_enum loglevel;


}  // namespace tmss




//  SPDLOG_LOGGER_CALL(tmss::Logger::Instance()->get_logger(), level, fmt, args);
#ifndef tmss_log
#define tmss_log(level, fmt, ...) \
    if (level >= tmss::loglevel) { \
        tmss::Logger::Instance()->get_logger()->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
        level, fmt, ##__VA_ARGS__); \
    }
#endif

#define tmss_trace(msg, ...) tmss_log(spdlog::level::trace, msg,  ##__VA_ARGS__)
#define tmss_debug(msg, ...) tmss_log(spdlog::level::debug, msg,  ##__VA_ARGS__)
#define tmss_info(msg, ...)  tmss_log(spdlog::level::info,  msg,   ##__VA_ARGS__)
#define tmss_warn(msg, ...) tmss_log(spdlog::level::warn, msg,  ##__VA_ARGS__)
#define tmss_error(msg, ...) tmss_log(spdlog::level::err, msg,  ##__VA_ARGS__)




