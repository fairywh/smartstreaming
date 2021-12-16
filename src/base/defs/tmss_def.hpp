/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#pragma once
#include <assert.h>
#include <memory>
namespace tmss {
// free the p and set to NULL.
// p must be a T*.
#define freep(p) \
        if (p) { \
            delete p; \
            p = NULL; \
        } \
        (void)0
// please use the freepa(T[]) to free an array,
// or the behavior is undefined.
#define freepa(pa) \
        if (pa) { \
            delete[] pa; \
            pa = NULL; \
        } \
        (void)0

struct OptionContext {
    std::string dll_file;
};

enum EStatusSource {
    ESourceInit = 0,
    ESourceStart = 1
};

enum EInputType {
    EInputPublish = 0,
    EInputOrigin = 1
};

enum EOutputType {
    EOutputPlay = 0,
    EOutputForawrd = 1
};

enum EStatusOutput {
    EOutputInit = 0,
    EOutputStart = 1
};

// default vhost of rtmp
#define CONSTS_RTMP_DEFAULT_VHOST "__defaultVhost__"
// default port of rtmp
#define CONSTS_RTMP_DEFAULT_PORT "1935"

// flv
#define CONSTS_FLV_DEFAULT_PORT "80"
}  // namespace tmss

