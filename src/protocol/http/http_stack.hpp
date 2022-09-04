/****************************************
 * Copyright [2020] <Tencent, China>
 * function: http parse and build response
 *
 * author: rainwu@tencent.com
 *
 * date: 2021/04/14
 ****************************************/
#pragma once
#include <string>
#include <map>
#include "util/util.hpp"
#include <defs/err.hpp>

namespace tmss {
extern std::map<int, std::string> http_status_detail;

class CHttp {
 private:
    std::string method_;
    std::string url_;
    std::string query_string_;
    std::string data_;
    unsigned int content_len_;
    std::string name_;
    std::string folder_;
    std::string path_;
    std::string forwarded_client_ip_;        // X-Forwarded-For
    std::string request_host_;
    std::string ext_;   // streamid.flv, ext is flv
    bool is_transcode_;     //  to do

    int response_status;

 public:
    CHttp();
    ~CHttp();
    unsigned int getContentLen();
    std::string Url()   { return path_ + "?" + query_string_;}
    std::string Data() { return data_; }   // 如果是parseRequst，就是输入的data；
    // 如果是parseHttpResponse，就是返回的data
    std::string Path()  {   return path_;}
    std::string QueryString()   {   return query_string_;}
    std::string Name()          {   return name_;}
    std::string Folder()                {return folder_;}
    std::string ForwardedIp()      { return forwarded_client_ip_;}
    std::string Host()          { return request_host_;}
    std::string ext()           { return ext_; }
    bool is_transcode()     { return is_transcode_; }
    int parse_request(const std::string& data);
    int parsePostRequest(const std::string& data);
    int parseGetRequest(const std::string & data, int &is_crossdomain_request);
    int checkHttpInputWithLength(const std::string & data);
    int checkHttpInputSimple(const std::string & data);
    int checkRequest(const std::string& data);
    int checkResponse(const std::string& data);
    int buildResponse(const std::string& rsp_data, std::string& str_rsp);
    int buildHttpGetRequest(const std::string& param,
        std::string& str_req,
        const std::string &host,
        const std::string &path,
        const std::string &name);
    int buildHttpPostRequest(const std::string& req_data,
        std::string& str_req,
        const std::string &host,
        const std::string &path,
        const std::string &name);
    int parseHttpResponse(const std::string& data);
    void DomainPolicy(std::string& crossdomain_info);
    int buildRawResponse(int code, std::string& str_rsp);

    int get_response_status(const std::string& data);
};  //  end of CHttp
}   //  namespace tmss

