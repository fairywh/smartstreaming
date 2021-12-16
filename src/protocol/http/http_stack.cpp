/*
 * TMSS
 * Copyright (c) 2020 rainwu
 */

#include "http/http_stack.hpp"
#include <log/log.hpp>

namespace tmss {
std::map<int, std::string> http_status_detail = {
        {200, "HTTP 200 OK"}, {404, "404 Not Found"}
};

CHttp::CHttp() {
    response_status = 0;
}

CHttp::~CHttp() {
}

unsigned int CHttp::getContentLen() {
    return content_len_;
}

int CHttp::parseRequest(const std::string& data) {
    // find space
    std::size_t found = data.find(" ");
    if (found == std::string::npos) {
        return -1;
    }

    // get method
    method_ = data.substr(0, found);

    tmss_info("method={}", method_);

    if (method_ == "POST") {
        return parsePostRequest(data);
    } else if (method_ == "GET") {
        int is_crossdomain_request = 0;
        return parseGetRequest(data, is_crossdomain_request);
    } else {
        return -1;
    }
    return 0;
}

int CHttp::parsePostRequest(const std::string& data) {
    // find space
    std::size_t found = data.find(" ");
    if (found == std::string::npos) {
        return -1;
    }

    // get method
    method_ = data.substr(0, found);
    if (method_ != "POST") {
        return -1;
    }

    unsigned int start = 0;
    // unsigned int end = 0;

    start = found + 1;
    found = data.find(" ", start);
    if (found == std::string::npos) {
        return -1;
    }
    url_ = data.substr(start, found);

    std::string find_string = "Content-Length:";
    found = data.find(find_string);
    if (found == std::string::npos) {
        return -1;
    }
    start = found + find_string.length();

    find_string = "\r\n";
    found = data.find("\r\n");
    if (found == std::string::npos) {
        return -1;
    }

    std::string str_len = data.substr(start, found - start);

    // TODO(tencent): atoi is not safe, must to replace by other functions
    content_len_ = atoi(str_len.c_str());

    find_string = "\r\n\r\n";
    found = data.find(find_string);
    if (found == std::string::npos) {
        return -1;
    }

    start = found + 4;

    if (data.length() < start + content_len_) {
        return -1;
    }
    data_ = data.substr(start, content_len_);

    return 0;
}

int CHttp::parseGetRequest(const std::string & data, int &is_crossdomain_request) {
    if (data.find("crossdomain.xml") != std::string::npos) {
        is_crossdomain_request = 1;
        return 0;
    } else if (data.find("<policy-file-request/>") != std::string::npos) {
        is_crossdomain_request = 1;
        return 0;
    } else {
        is_crossdomain_request = 0;
        // find space
        std::size_t path_found = data.find("/");

        if (path_found == std::string::npos) {
           return -1;
        }

        // get method
        if (method_.empty()) {
            method_ = data.substr(0, 3);
        }

        if (method_ != "GET") {
           return -1;
        }
        std::size_t params_found = data.find("?");
        std::size_t http_found = data.find(" HTTP");

        if (http_found == std::string::npos) {
           return -1;
        }

        // printf("data=%s,found=%u\n",data.c_str(),found);
        if (params_found == std::string::npos) {
            path_ = data.substr(path_found, http_found-path_found);

            query_string_ = "";
        } else {
            path_ = data.substr(path_found, params_found-path_found);
            unsigned int start = params_found+1;
            query_string_ = data.substr(start, http_found-start);
        }

        std::size_t name_found = path_.find_last_of("/");
        if (name_found == std::string::npos) {
            name_ = path_;
            folder_ = "";
        } else {
            folder_ = path_.substr(1, name_found - 1);
            name_ = path_.substr(name_found + 1);
        }

        std::size_t ext_found = name_.find_last_of(".");
        if (ext_found == std::string::npos) {
            ext_ = name_;
        } else {
            ext_ = name_.substr(ext_found + 1);
        }
        //  X-Forwarded-For
        std::string forwarded_string = "X-Forwarded-For: ";
        std::size_t forwarded_start_found = data.find(forwarded_string);
        if (forwarded_start_found == std::string::npos) {
        } else {
            unsigned int forwarded_start = forwarded_start_found + forwarded_string.length();

            std::size_t forwarded_end_found = data.find("\r\n", forwarded_start);

            if (forwarded_end_found == std::string::npos) {
            } else {
                forwarded_client_ip_ = data.substr(forwarded_start, forwarded_end_found - forwarded_start);
            }
        }

        //  host
        std::string request_host_string = "Host: ";
        std::size_t request_host_start_found = data.find(request_host_string);
        if (request_host_start_found == std::string::npos) {
        } else {
            unsigned int request_host_start = request_host_start_found + request_host_string.length();

            std::size_t request_host_end_found = data.find("\r\n", request_host_start);

            if (request_host_end_found == std::string::npos) {
            } else {
                request_host_ = data.substr(request_host_start, request_host_end_found - request_host_start);
            }
        }
        return 0;
    }
}

int CHttp::checkHttpInputWithLength(const std::string & data) {
    std::string find_string = "Content-Length:";
    std::size_t found = data.find(find_string);
    if (found == std::string::npos) {
        return 0;
    }
    unsigned int start = 0;

    start = found + find_string.length();
    find_string = "\r\n";
    found = data.find(find_string, start);
    if (found == std::string::npos) {
        return 0;
    }

    std::string str_len = data.substr(start, found - start);
    content_len_ = atoi(str_len.c_str());

    find_string = "\r\n\r\n";
    found = data.find(find_string);
    if (found == std::string::npos) {
        return 0;
    }

    start = found + find_string.length();
    if (start + content_len_ > data.length()) {
        return 0;
    }

    return (start + content_len_);
}

int CHttp::checkHttpInputSimple(const std::string & data) {
    std::string find_string = "\r\n\r\n";
    std::size_t found = data.find(find_string);
    if (found == std::string::npos) {
        return 0;
    } else {
        return found + find_string.length();
    }
}

int CHttp::checkRequest(const std::string& data)  {
    std::string find_string = "Content-Length:";
    std::size_t found = data.find(find_string);
    if (found == std::string::npos) {
        return checkHttpInputSimple(data);
    } else {
        return checkHttpInputWithLength(data);
    }
}


int CHttp::checkResponse(const std::string& data) {
    std::string find_string = "Content-Length:";
    std::size_t found = data.find(find_string);
    if (found == std::string::npos) {
        return checkHttpInputSimple(data);
    } else {
        return checkHttpInputWithLength(data);
    }
}

int CHttp::buildResponse(const std::string& rsp_data,  std::string& str_rsp) {
    uint32_t content_len = rsp_data.size();
    // LOG(INFO) << "content_len: " << content_len;
    // util::Log(util::DEBUG, "content_len: %d\n", content_len);

    str_rsp = "";
    str_rsp += "HTTP/1.1 200 OK\r\n";
    str_rsp += "Content-Type: text/plain\r\n";
    str_rsp += "Content-Length: " + tmss::GetStrFromUInt32(content_len) + "\r\n";
    str_rsp += "Connection: Keep-Alive\r\n";
    str_rsp += "\r\n";

    str_rsp.append(rsp_data);

    return 0;
}

int CHttp::buildHttpGetRequest(const std::string& param,
    std::string& str_req,
    const std::string &host,
    const std::string &path,
    const std::string& name) {
    // str_req = str_req +  "GET " + req_data + " HTTP/1.1\r\n";
    // str_req += "Accept: */*\r\nAccept-Charset: GBK,utf-8*\r\nAccept-Language: zh-CN,zh\r\nHost: ";
    str_req = str_req + "GET /" + path + "/" + name + "?" + param;
    str_req += " HTTP/1.1\r\nUser-Agent: curl/7.15.1 (linux) OpenSSL/0.9.8a zlib/1.2.3 libidn/0.6.0";
    // str_req += "\r\nX-Callback-Host: http://";
    str_req += "\r\nHost: ";
    str_req += host;
    // str_req += "\r\nX-Callback-App: douyu\r\nX-Callback-Random: 35756";
    // str_req += "\r\nHost: ";
    // str_req += "OpenCallback";
    str_req += "\r\nAccept: */*\r\n\r\n";  // "Connection: Keep-Alive\r\n\r\n";
    return 0;
}
int CHttp::buildHttpPostRequest(const std::string& req_data,
    std::string& str_req,
    const std::string &host,
    const std::string &path,
    const std::string &name) {
    /***
        POST /DEMOWebServices2.8/Service.asmx/CancelOrder HTTP/1.1
        Host: api.efxnow.com
        Content-Type: application/x-www-form-urlencoded
        Content-Length: length
        {}
    // **/
    uint32_t content_len = req_data.size();
    // LOG(INFO) << "content_len: " << content_len;
    // util::Log(util::DEBUG, "content_len: %d\n", content_len);
    str_req = "";
    str_req += "POST ";
    str_req += path;
    str_req += name;
    str_req += " HTTP/1.1";
    // str_req += "\r\nX-Callback-Host: ";
    str_req += "\r\nHost: ";
    str_req += host;
    // str_req += "\r\nX-Callback-App: douyu\r\nX-Callback-Random: 35756";
    // str_req += "\r\nHost: ";
    // str_req += "OpenCallback";
    str_req += "\r\n";
    str_req += "Content-Type: text/plain\r\n";
    str_req += "Content-Length: " + tmss::GetStrFromUInt32(content_len) + "\r\n";
    // str_req += "Connection: Keep-Alive\r\n";
    str_req += "\r\n";
    str_req.append(req_data);
    return 0;
}

int CHttp::parseHttpResponse(const std::string& data) {
    get_response_status(data);
    if (strstr(data.c_str(), "Content-Length: ") != NULL) {
        /*
            0x0000:  4500 0127 1d67 4000 3a06 a40b ac1b c5a9  E..'.g@.:.......
                0x0010:  0a85 0215 0050 7f73 974a e600 fe8c 3d73  .....P.s.J....=s
                0x0020:  8018 0007 c4c4 0000 0101 080a 02df fc4c  ...............L
                0x0030:  2aab 25a6 4854 5450 2f31 2e31 2032 3030  *.%.HTTP/1.1.200
                0x0040:  204f 4b0d 0a43 6f6e 6e65 6374 696f 6e3a  .OK..Connection:
                0x0050:  206b 6565 702d 616c 6976 650d 0a4b 6565  .keep-alive..Kee
                0x0060:  702d 416c 6976 653a 2074 696d 656f 7574  p-Alive:.timeout
                0x0070:  3d36 302c 206d 6178 3d31 3032 340d 0a53  =60,.max=1024..S
                0x0080:  6572 7665 723a 2051 5a48 5454 502d 322e  erver:.QZHTTP-2.
                0x0090:  3338 2e32 300d 0a44 6174 653a 2046 7269  38.20..Date:.Fri
                0x00a0:  2c20 3236 204a 756e 2032 3031 3520 3132  ,.26.Jun.2015.12
                0x00b0:  3a34 323a 3239 2047 4d54 0d0a 636f 6e74  :42:29.GMT..cont
                0x00c0:  656e 742d 7479 7065 3a20 7465 7874 2f70  ent-type:.text/p
                0x00d0:  6c61 696e 3b20 6368 6172 7365 743a 7574  lain;.charset:ut
                0x00e0:  662d 380d 0a43 6f6e 7465 6e74 2d4c 656e  f-8..Content-Len
                0x00f0:  6774 683a 2034 340d 0a0d 0a51 5a4f 7574  gth:.44....QZOut
                0x0100:  7075 744a 736f 6e3d 7b22 7265 7422 3a2d  putJson={"ret":-
                0x0110:  332c 226d 7367 223a 2269 6e70 7574 2065  3,"msg":"input.e
                0x0120:  7272 6f72 227d 3b                        rror"};
        // */
        std::string find_string = "Content-Length:";
        std::size_t found = data.find(find_string);
        if (found == std::string::npos) {
           return -1;
        }
        unsigned int start = 0;
        start = found + find_string.length();

        find_string = "\r\n";
        found = data.find("\r\n", start);
        if (found == std::string::npos) {
           return -1;
        }

        std::string str_len = data.substr(start, found - start);

        // TODO(tencent): atoi is not safe, must to replace by other functions
        content_len_ = atoll(str_len.c_str());

        find_string = "\r\n\r\n";
        found = data.find(find_string);
        if (found == std::string::npos) {
           return -1;
        }

        start = found + 4;

        if (data.length() < start + content_len_) {
           return -1;
        }

        // if (data.length() > start + content_len_)
        //{
            // data_ = data.substr(start, content_len_);
        //}
        // else
        //{
        data_ = data.substr(start);
        //}

        // printf("content_len_=%d,data.length=%d/%d,data_=%s\n",content_len_,data_.length(),data.length(),data_.c_str());

        return 0;
    } else if (strstr(data.c_str(), "Transfer-Encoding: chunked") != NULL) {
        /*
            0x0000:  4564 0147 7023 4000 3906 60e3 0a82 582b  Ed.Gp#@.9.`...X+
            0x0010:  0a85 0215 0050 ee36 da91 ab5d 9c60 6ce1  .....P.6...].`l.
            0x0020:  8018 0007 7822 0000 0101 080a ed25 db80  ....x".......%..
            0x0030:  2aab c995 4854 5450 2f31 2e31 2032 3030  *...HTTP/1.1.200
            0x0040:  204f 4b0d 0a43 6f6e 6e65 6374 696f 6e3a  .OK..Connection:
            0x0050:  2063 6c6f 7365 0d0a 5365 7276 6572 3a20  .close..Server:.
            0x0060:  515a 4854 5450 2d32 2e33 382e 3332 0d0a  QZHTTP-2.38.32..
            0x0070:  4461 7465 3a20 4672 692c 2032 3620 4a75  Date:.Fri,.26.Ju
            0x0080:  6e20 3230 3135 2031 323a 3435 3a31 3620  n.2015.12:45:16.
            0x0090:  474d 540d 0a43 6f6e 7465 6e74 2d54 7970  GMT..Content-Typ
            0x00a0:  653a 2061 7070 6c69 6361 7469 6f6e 2f78  e:.application/x
            0x00b0:  2d6a 6176 6173 6372 6970 743b 2063 6861  -javascript;.cha
            0x00c0:  7273 6574 3d75 7466 2d38 0d0a 5472 616e  rset=utf-8..Tran
            0x00d0:  7366 6572 2d45 6e63 6f64 696e 673a 2063  sfer-Encoding:.c
            0x00e0:  6875 6e6b 6564 0d0a 0d0a 3537 0d0a 515a  hunked....57..QZ
            0x00f0:  4f75 7470 7574 4a73 6f6e 3d7b 2265 6d22  OutputJson={"em"
            0x0100:  3a31 3030 342c 226d 7367 223a 22e5 afb9  :1004,"msg":"...
            0x0110:  e4b8 8de8 b5b7 efbc 8ce7 b3bb e7bb 9fe5  ................
            0x0120:  bc82 e5b8 b8ef bc8c e8af b7e7 a88d e590  ................
            0x0130:  8ee9 878d e8af 9520 3a29 222c 2273 223a  ........:)","s":
            0x0140:  2266 227d 3b0d 0a                        "f"};..
        // */
        std::string find_string = "Transfer-Encoding: chunked";
        std::size_t found = data.find(find_string);
        if (found == std::string::npos) {
           return -1;
        }
        unsigned int start = 0;
        start = found + find_string.length();

        find_string = "\r\n\r\n";
        found = data.find(find_string);
        if (found == std::string::npos) {
           return -1;
        }

        start = found + 4;

        find_string = "\r\n";
        found = data.find(find_string, start);

        std::string str_len = data.substr(start, found - start);

        // TODO(tencent): atoi is not safe, must to replace by other functions
        // content_len_ = atoll(str_len.c_str());
        sscanf(str_len.c_str(), "%x", &content_len_);

        start = found+2;  //  data

        if (data.length() < start + content_len_) {
           return -1;
        }

        data_ = data.substr(start, content_len_);

        return 0;
    } else {
        // continuous data
        std::string find_string = "\r\n\r\n";
        std::size_t found = data.find(find_string);
        if (found == std::string::npos) {
           return -1;
        }
        unsigned int start = found + find_string.length();
        data_ = data.substr(start);
        return 0;
    }
}

void CHttp::DomainPolicy(std::string & crossdomain_info) {
    std::string strCrossdomainXML = "";
    strCrossdomainXML += "<?xml version=\"1.0\"?>\r\n";
    // strCrossdomainXML += "<!DOCTYPE cross-domain-policy \r\n";
    // strCrossdomainXML += "SYSTEM \"http:// www.macromedia.com/xml/dtds/cross-domain-policy.dtd\"\r\n>";
    strCrossdomainXML += "<cross-domain-policy>\r\n";
    // strCrossdomainXML += "<site-control permitted-cross-domain-policies=\"all\"/>\r\n";
    // dstrCrossdomainXML += "<allow-access-from domain=\"*.tencent.com\" secure=\"false\"/>\r\n";
    strCrossdomainXML += "<allow-access-from domain=\"*.qq.com\" secure=\"false\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"*.paipai.com\" secure=\"false\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"*.soso.com\" secure=\"false\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"*.cm.com\" secure=\"false\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"*.expo2010china.com\" secure=\"false\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"*.expo.cn\" secure=\"false\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"*.www.expo2010.cn\" secure=\"false\"/>\r\n";
    strCrossdomainXML += "<allow-access-from domain=\"qzonestyle.gtimg.cn\" secure=\"false\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"203.195.168.66\" to-ports=\"*\"/>\r\n";
    // strCrossdomainXML += "<allow-http-request-headers-from domain=\"*.qq.com\" headers=\"*\" secure=\"true\"/>\r\n";
    // strCrossdomainXML += "<allow-access-from domain=\"*\" to-ports=\"*\"/>\r\n";
    // strCrossdomainXML += "<allow-http-request-headers-from domain=\"*\" headers=\"*\" secure=\"false\"/>\r\n";
    strCrossdomainXML += "</cross-domain-policy>\r\n";

    std::string strrsp;
    strrsp = "HTTP/1.1 200 OK\r\n";
    strrsp += "Content-Type: text/xml\r\n";
    strrsp += "Last-Modified: Tue, 07 Apr 2010 14:37:56 GMT\r\n";
    strrsp += "Accept-Ranges: bytes\r\n";
    // strrsp += "Content-Length: ";
    // strrsp += to_string<int>(strCrossdomainXML.size());
    strrsp += "\r\n\r\n";
    strrsp += strCrossdomainXML;
     // enqueue_2_ccd(strCrossdomainXML2.c_str(), strCrossdomainXML2.size(), flow);
     crossdomain_info = strrsp;
     //  close
     // m_SpStat.m_nNumOnline--;
}

int CHttp::buildRawResponse(int code, std::string & str_rsp) {
    if (code == 200) {
        str_rsp = "";
        str_rsp += "HTTP/1.1 200 OK\r\n";
        str_rsp += "Content-Type: text/plain\r\n";
        str_rsp += "Content-Length: 0\r\n";
        str_rsp += "Connection: Keep-Alive\r\n";
        str_rsp += "\r\n";
    } else if (code == 404) {
        str_rsp = "";
        str_rsp += "HTTP/1.1 404 Not Found\r\n";
        str_rsp += "Content-Type: text/plain\r\n";
        str_rsp += "Content-Length: 0\r\n";
        str_rsp += "Connection: close\r\n";
        str_rsp += "\r\n";
    } else {
        str_rsp = "";
        str_rsp += "HTTP/1.1 403 Forbidden\r\n";
        str_rsp += "Content-Type: text/plain\r\n";
        str_rsp += "Content-Length: 0\r\n";
        str_rsp += "Connection: close\r\n";
        str_rsp += "\r\n";
    }
    return 0;
}
int CHttp::get_response_status(const std::string& data) {
    if (response_status > 0) {
        return response_status;
    }
    std::string header = data.substr(0, strlen("HTTP"));
    if (header != "HTTP") {
        return -1;
    }
    std::string found_status_start = " ";
    std::size_t found = data.find(found_status_start);
    if (found == std::string::npos) {
       return -1;
    }
    unsigned int start = found + found_status_start.length();
    found = data.find(" ", start);
    if (found == std::string::npos) {
       return -1;
    }
    std::string str_len = data.substr(start, found - start);
    response_status = atoll(str_len.c_str());

    return response_status;
}
}   //  namespace tmss


