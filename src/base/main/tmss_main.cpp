/*
 * TMSS
 * Copyright (c) 2021 rainwu
 */

#include <sys/wait.h>
#include <iostream>
#include <dll/tmss_dll.hpp>
#include <defs/tmss_def.hpp>
#include <defs/err.hpp>
#include <coroutine.hpp>
#include <util/timer.hpp>
#include <media_source_handler.hpp>
#include <log/log.hpp>

//  extern tmss_dll_func_t tmss_dll;

void show_help() {
    std::cout << "help." << std::endl;
}

int parse_options(int &i, char **argv, tmss::OptionContext& option_ctx) {
    char* p = argv[i];
    if (*p++ != '-') {
        show_help();
        exit(0);
    }
    while (*p) {
        switch (*p++) {
            case '?':
            case 'h':
                show_help();
                exit(0);
            case 'l':
            case 'L':
                if (*p) {
                    option_ctx.dll_file = p;
                    continue;
                }
                if (argv[++i]) {
                    option_ctx.dll_file = argv[i];
                    continue;
                }
                return -1;
            default:
                return -2;
        }
    }
    return 0;
}

int run_as_daemon() {
    int pid = fork();

    if (pid < 0) {
        tmss_error("fork failed pid={}", pid);
        return -1;
    }

    // grandpa
    if (pid > 0) {
        int status = 0;
        if (waitpid(pid, &status, 0) == -1) {
            // tmss_error("waitpid failed pid:{}", pid);
        }
        exit(0);
    }

    // father
    pid = fork();

    if (pid < 0) {
        // tmss_error("fork failed pid:{}", pid);
        return -1;
    }

    if (pid > 0) {
        exit(0);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    std::cout << "hello-tmss" << std::endl;
    if (st_init() != 0) {
        std::cout << "st init failed" << std::endl;
        return -1;
    }

    run_as_daemon();

    tmss::Logger::Instance()->set_logger("./log/tmss.log",
                  1000, 10);

    tmss_info("st_get_eventsys_name={}", st_get_eventsys_name());
    // decode argument
    // read from config
    tmss::OptionContext option_ctx;
    for (int i = 1; i < argc; i++) {
        parse_options(i, argv, option_ctx);
    }
    if (!option_ctx.dll_file.empty()) {
        std::cout << "dll=" << option_ctx.dll_file << std::endl;
        tmss_info("dll={}", option_ctx.dll_file.c_str());
        load_dll(option_ctx.dll_file.c_str());
        tmss_dll.init_user_control(argc, argv);
    } else {
        std::cout << "no dll" << std::endl;
        if (tmss::init_default_handler(argc, argv) != error_success) {
            tmss_error("init failed");
            return -1;
        }
    }
    tmss_info("server start");
    // different server can share the same channel or input

    std::shared_ptr<tmss::StTimer> st = std::make_shared<tmss::StTimer>();
    st->run();
    while (true) {
        st_usleep(1000 * 1000);
    }
    return 0;
}

