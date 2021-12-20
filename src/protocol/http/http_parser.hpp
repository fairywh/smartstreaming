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

#include <memory>
#include <list>
#include <utility>
#include <string>

#include <io/io.hpp>
#include <parser.hpp>

namespace tmss {
class HttpRequest : public Request {
 public:
    std::list<std::pair<std::string, std::string> > headers;
    bool ip_domain = false;
    std::string ext;
};

class HttpResponse : public Response {
 public:
    int http_status;
    std::list<std::pair<std::string, std::string> > headers;
};

class HttpParser {
 public:
    /**
     * 从io中解析http response内容
     */
    int parse_response(std::shared_ptr<IReader> reader, std::shared_ptr<HttpResponse> &response);

    /**
     * 从io中解析请求
     * @param reader
     * @param req
     * @return
     */
    int parse_request(std::shared_ptr<IReader> reader, std::shared_ptr<HttpRequest> &req);
};

};  // namespace tmss
