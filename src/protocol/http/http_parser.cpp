/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/29.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#include <http_parser.hpp>
#include <http_stack.hpp>
#include <defs/err.hpp>
#include <log/log.hpp>

namespace tmss {
int HttpParser::parse_response(std::shared_ptr<IReader> reader,
        std::shared_ptr<HttpResponse> &response) {
    int ret = error_success;
    return ret;
}

int HttpParser::parse_request(std::shared_ptr<IReader> reader,
        std::shared_ptr<HttpRequest> &req) {
    std::shared_ptr<HttpRequest> new_req = std::make_shared<HttpRequest>();
    int ret = error_success;
    char buff[1024];
    int size = sizeof(buff);

    CHttp http;
    int read_size = 0;
    int total_size = 0;
    while ((read_size = reader->read(buff, size - 1)) > 0) {
        if (read_size > sizeof(buff)) {
            // error
            tmss_error("read buffer error,read_size={}", read_size);
            ret = error_socket_read;
            return ret;
        }
        buff[read_size] = '\0';
        size -= read_size;
        total_size += read_size;
        if (http.checkRequest(std::string(buff)) > 0) {
            tmss_info("http request complete,{}", buff);
            break;
        }
    }

    if (read_size < 0) {
        tmss_error("read error,ret={}", read_size);
        ret = error_socket_read;
        return ret;
    } else if (read_size == 0) {
        tmss_info("read stop");
        if (total_size == 0) {
            tmss_warn("read none");
            ret = error_socket_already_closed;
            return ret;
        }
    }

    // complete
    // default, to do
    new_req->type = ERequestTypePlay;

    http.parse_request(std::string(buff));
    new_req->vhost = http.Host();
    new_req->name = http.Name();
    new_req->path = http.Folder();
    new_req->params = http.QueryString();
    new_req->ext = http.ext();
    new_req->is_transcode = http.is_transcode();

    std::vector<std::string> tmp_querys;
    split_string(new_req->params, "&", tmp_querys);

    for (auto & key_value : tmp_querys) {
        std::vector<std::string> kv;
        split_string(key_value, "=", kv);

        if (kv.size() == 2) {
            new_req->params_map[kv[0]] = url_decode(kv[1]);
        }
    }

    req = new_req;

    tmss_info("create new request");
    return ret;
}
}  // namespace tmss