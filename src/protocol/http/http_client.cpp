/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#include <protocol/http/http_client.hpp>

#include <utility>

#include <defs/err.hpp>
#include <log/log.hpp>
#include <format/demux.hpp>
#include <util/util.hpp>
#include <http_stack.hpp>

namespace tmss {
HttpClient::HttpClient() {
}

int HttpClient::cycle() {
    int ret = error_success;
    return ret;
}

int HttpClient::request(const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& param,
        std::shared_ptr<IDeMux> demux) {
    int ret = error_success;
    std::string request;    // packet origin_url
    CHttp http_request;
    http_request.buildHttpGetRequest(param,
        request,
        origin_host,
        origin_path,
        stream);
    tmss_info("request={}", request);
    // to do
    ret = conn->write(request.c_str(), request.length());
    if (ret < 0) {
        tmss_error("ingest send error, {}", ret);
        return ret;
    }
    // to do
    char response[1024] = {0};
    int buffer_size = sizeof(response);
    int once_read_size = 0;
    int total_read_size = 0;
    // parse response
    CHttp http_response;
    char * pos_response = response;
    //  while ((once_read_size = conn->read(pos_response, 1)) > 0) {
    while ((once_read_size = demux->read_packet(pos_response, 1)) > 0) {
        pos_response += once_read_size;
        // response[total_read_size] = '\0';
        buffer_size -= once_read_size;
        if (buffer_size <= 0) {
            tmss_error("no buffer");
            ret = error_socket_read;
            return ret;
        }
        total_read_size += once_read_size;
        if (http_response.checkResponse(response) > 0) {
            tmss_info("http response complete,{}", response);
            break;
        }
        tmss_info("response, {}, {}",
            total_read_size, PrintBuffer(response, total_read_size));
    }
    if (once_read_size < 0) {
        tmss_error("ingest read error, {}", once_read_size);
        ret = error_socket_read;
        return ret;
    } else if (once_read_size == 0) {
        tmss_info("read stop");
        if (total_read_size == 0) {
            tmss_warn("read none");
            ret = error_socket_already_closed;
            return ret;
        }
    }

    tmss_info("http_first_packet, {}, {}",
        total_read_size, PrintBuffer(response, total_read_size));

    std::string http_first_packet(response, total_read_size);
    http_response.parseHttpResponse(http_first_packet);
    int status = http_response.get_response_status(http_first_packet);
    tmss_info("origin response {}", status);
    if (status != 200) {
        ret = error_ingest_no_input;
        tmss_error("origin response error {}", status);
        return ret;
    }
    // this have some data, (no need, can be delete)`
    std::string data_header = http_response.Data();
    demux->on_ingest(http_response.getContentLen(), data_header);
    return error_success;
}

int HttpClient::read_data(char* buf, int size) {
    // int read_size = input_conn->read(buff, wanted_size);
    int read_size = io_buffer->read_bytes(buf, size);
    if (read_size < 0) {
        tmss_error("fetch stream failed, ret={}", read_size);
        return read_size;
    }
    tmss_info("read data, {}/{}, {}", read_size, size, PrintBuffer(buf, read_size));
    return read_size;
}

int HttpClient::write_data(const char* buf, int size) {
    return conn->write(buf, size);
}

}   // namespace tmss
