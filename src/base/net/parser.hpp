/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/5
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#pragma once
#include <string>


namespace tmss {

enum ERequestType {
    ERequestTypePlay = 0,
    ERequestTypePublish = 1,
    ERequestTypeApi = 2
};

class IDeMux;
class IMux;
class Request {
 public:
    virtual ~Request() = default;

    std::string vhost;
    std::string path;
    std::string name;
    std::string url;
    std::string params;
    ERequestType type;
    std::string format;   // usually format is ext in http

    std::string to_str() {
        return vhost + "|" + path + "|" + name + "|" + params;
    }
};

class Response {
 public:
    int content_length         = -1;
    bool chunk_size            = false;
};

}  // namespace tmss

