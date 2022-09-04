/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/5
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include "util/util.hpp"
#include <sys/time.h>
#include <unistd.h>
#include <defs/tmss_def.hpp>
#include <log/log.hpp>

namespace tmss {
std::string GetStrFromUInt32(uint32_t value) {
    char buf[100] = {0};
    int val_len = 0;

    val_len = snprintf(buf, sizeof(buf) - 1, "%u", value);

    return std::string(buf, val_len);
}

std::string GetStrFromInt64(int64_t value) {
    char buf[100] = {0};
    int val_len = 0;

    val_len = snprintf(buf, sizeof(buf) - 1, "%ld", value);

    return std::string(buf, val_len);
}

int64_t GetInt64FromString(std::string str) {
    std::stringstream ss;
    ss << str;
    int64_t ret = 0;
    ss >> ret;
    return ret;
}

int64_t GetNowTimeByMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)(tv.tv_sec) * 1000 + (int64_t)(tv.tv_usec)/1000);
}

std::string PrintBuffer(const void *buffer, unsigned int size) {
    std::string result;
    for (unsigned int tmp = 0; tmp < size; tmp++) {
        char temp[4];
        void *const_p = const_cast<void*>(buffer);
        char *p = static_cast<char*>(const_p);
        if (*(p + tmp) < 0) {
            char b = (*(p + tmp) >> 4) & 0x0f;
            char c = *(p + tmp) & 0x0f;
            snprintf(temp, sizeof(temp), "%x%x ", b, c);
        } else {
            snprintf(temp, sizeof(temp), "%02x ", *(p + tmp));
        }
        result += temp;
    }
    return result;
}

std::string generate_tc_url(std::string ip, std::string vhost, std::string app, std::string port,
    std::string param) {
    std::string tcUrl = "rtmp://";

    if (vhost == CONSTS_RTMP_DEFAULT_VHOST) {
        tcUrl += ip;
    } else {
        tcUrl += vhost;
    }

    if (port != CONSTS_RTMP_DEFAULT_PORT) {
        tcUrl += ":";
        tcUrl += port;
    }

    tcUrl += "/";
    tcUrl += app;
    tcUrl += param;

    return tcUrl;
}

void random_generate(char* bytes, int size) {
    static bool _random_initialized = false;
    if (!_random_initialized) {
        srand(0);
        _random_initialized = true;
        tmss_info("srand initialized the random.");
    }

    for (int i = 0; i < size; i++) {
        // the common value in [0x0f, 0xf0]
        bytes[i] = 0x0f + (rand() % (256 - 0x0f - 0x0f));
    }
}

/*******************************************************************************
 * 函数名称  : SplitInt
 * 功能描述  : 分割字符串为整型
 * 函数参数  : const string & srcStr
 * 函数参数  : const string & splitStr
 * 函数参数  : vector<int> & destVec
 * 返 回 值  : int
 *******************************************************************************/
int split_int(const std::string &srcStr, const std::string &splitStr, std::vector<int> &destVec) {
    if (srcStr.length() == 0) {
        return 0;
    }
    size_t oldPos = 0;
    size_t newPos = 0;
    std::string tempData;
    while (1) {
        newPos = srcStr.find(splitStr, oldPos);
        if (newPos != std::string::npos) {
            tempData = srcStr.substr(oldPos, newPos - oldPos);
            destVec.push_back(from_string<int>(tempData));
            oldPos = newPos + splitStr.size();
        } else if (oldPos <= srcStr.size()) {
            tempData = srcStr.substr(oldPos);
            destVec.push_back(from_string<int>(tempData));
            break;
        } else {
            break;
        }
    }
    return 0;
}

/*******************************************************************************
* 函数名称  : SplitString
* 功能描述  : 分割字符串
* 函数参数  : const string & srcStr
* 函数参数  : const string & splitStr
* 函数参数  : vector<string> & destVec
* 返 回 值  : int
*******************************************************************************/
int split_string(const std::string &srcStr, const std::string &splitStr, std::vector<std::string> &destVec) {
    if (srcStr.length() == 0) {
        return 0;
    }
    size_t oldPos = 0;
    size_t newPos = 0;
    std::string tempData;
    while (1) {
        newPos = srcStr.find(splitStr, oldPos);
        if (newPos != std::string::npos) {
            tempData = srcStr.substr(oldPos, newPos - oldPos);
            destVec.push_back(tempData);
            oldPos = newPos + splitStr.size();
        } else if (oldPos <= srcStr.size()) {
            tempData = srcStr.substr(oldPos);
            destVec.push_back(tempData);
            break;
        } else {
            break;
        }
    }
    return 0;
}

unsigned char tp_hex(const unsigned char &x) {
        return x > 9 ? x - 10 + 'A': x + '0';
}

unsigned char from_hex(const unsigned char &x) {
        return isdigit(x) ? x-'0' : x-'A'+10;
}

std::string url_decode(const std::string &sIn) {
    std::string sOut;
    for (size_t ix = 0; ix < sIn.size(); ix++) {
        unsigned char ch = 0;
        if (sIn[ix] == '%') {
            ch = (from_hex(sIn[ix+1]) << 4);
            ch |= from_hex(sIn[ix+2]);
            ix += 2;
        } else if (sIn[ix] == '+') {
            ch = ' ';
        } else {
            ch = sIn[ix];
        }
        sOut += static_cast<char>(ch);
    }
    return sOut;
}
}   // namespace tmss

