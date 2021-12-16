#pragma once

#include <stdio.h>
/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/5
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#include <cstdint>
#include <string>
#include <sstream>

#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Min(a, b) (((a) < (b)) ? (a) : (b))

namespace tmss {
std::string GetStrFromUInt32(uint32_t value);

std::string GetStrFromInt64(int64_t value);

int64_t GetInt64FromString(std::string str);

int64_t GetNowTimeByMs();

template<typename T>
std::string to_string(T n) {
    std::ostringstream result;
    result << n;
    return result.str();
}

extern std::string PrintBuffer(const void *buffer, unsigned int size);

extern std::string generate_tc_url(std::string ip, std::string vhost, std::string app, std::string port,
    std::string param);

extern void random_generate(char* bytes, int size);
}   // namespace tmss

