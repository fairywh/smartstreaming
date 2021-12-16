/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#include <protocol/rtmp/rtmp_client.hpp>

#include <utility>

#include <defs/err.hpp>
#include <log/log.hpp>
#include <format/demux.hpp>
#include <util/util.hpp>
#include <rtmp_stack.hpp>

namespace tmss {
RtmpClient::RtmpClient() {
}

int RtmpClient::cycle() {
    int ret = error_success;
    return ret;
}

int RtmpClient::request(const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& param,
        std::shared_ptr<IDeMux> demux) {
    int ret = error_success;

    // generate_tc_url
    std::string request_url = generate_tc_url(origin_host,
        origin_host, origin_path, CONSTS_RTMP_DEFAULT_PORT,
        param);
    // connect
    int stream_id = 0;
    ret = connect(origin_host, request_url, stream_id, origin_path, conn);
    if (ret != error_success) {
        tmss_error("connect failed, ret:%d", ret);
        return ret;
    }

    // play
    ret = play(stream, stream_id);
    if (ret != error_success) {
        tmss_error("play failed, ret:%d", ret);
        return ret;
    }
    std::string data_header;
    demux->on_ingest(0, data_header);
    return ret;
}

int RtmpClient::read_data(char* buf, int size) {
    return protocol->recv_message(buf, size);
}

int RtmpClient::write_data(const char* buf, int size) {
    //  to do
    return protocol->send_message(buf, size);
}

int RtmpClient::connect(const std::string& origin_host,
        const std::string& request_url,
        int& stream_id,
        const std::string& path,
        std::shared_ptr<IClientConn> conn) {
    int ret = error_success;
    // handshake
    ret = handshake(conn);
    if (ret != error_success) {
        tmss_error("handshake failed ret={}", ret);
        return ret;
    }
    // connect
    ret = connect_app(path, request_url);
    if (ret != error_success) {
        tmss_error("conect_app failed ret={}", ret);
        return ret;
    }
    // create_stream
    ret = create_stream(stream_id);
    if (ret != error_success) {
        tmss_error("create_stream failed ret={}", ret);
        return ret;
    }
    return ret;
}

int RtmpClient::handshake(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    if ((ret = complex_handshake(conn)) != error_success) {
        tmss_warn("not support complex handshake");
        ret = simple_handshake(conn);
        return ret;
    } else {
        return ret;
    }
}

int RtmpClient::simple_handshake(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    RtmpHandshakeBytes handshake;
    if ((ret = handshake.create_c0c1()) != error_success) {
        return ret;
    }
    std::size_t nsize = conn->write(handshake.c0c1, 1537);
    if (nsize < 0) {
        tmss_error("write c0c1 failed. ret={}", ret);
        return ret;
    }

    if ((ret = handshake.read_s0s1s2(conn)) != error_success) {
        return ret;
    }

    // plain text required.
    if (handshake.s0s1s2[0] != 0x03) {
        ret = error_rtmp_handshake;
        tmss_error("handshake failed, plain text required. ret={}", ret);
        return ret;
    }

    if ((ret = handshake.create_c2()) != error_success) {
        return ret;
    }

    // for simple handshake, copy s1 to c2.
    // @see https://github.com/ossrs/srs/issues/418
    memcpy(handshake.c2, handshake.s0s1s2 + 1, 1536);

    if ((nsize = conn->write(handshake.c2, 1536)) != error_success) {
        tmss_error("simple handshake write c2 failed. ret={}", ret);
        return ret;
    }
    tmss_info("simple handshake success.");

    return ret;
}

int RtmpClient::complex_handshake(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;
    //  to do
    ret = error_rtmp_complex_handshake_not_support;
    return ret;
}

int RtmpClient::connect_app(std::string path, std::string tc_url) {
    int ret = error_success;
    std::shared_ptr<RtmpConnectAppPacket> pkt_connect_app = std::make_shared<RtmpConnectAppPacket>();
    pkt_connect_app->command_object->set("app", Amf0Any::str(path.c_str()));
    pkt_connect_app->command_object->set("flashVer", Amf0Any::str("WIN 15,0,0,239"));
    pkt_connect_app->command_object->set("swfUrl", Amf0Any::str());
    pkt_connect_app->command_object->set("tcUrl", Amf0Any::str(tc_url.c_str()));
    pkt_connect_app->command_object->set("fpad", Amf0Any::boolean(false));
    pkt_connect_app->command_object->set("capabilities", Amf0Any::number(239));
    pkt_connect_app->command_object->set("audioCodecs", Amf0Any::number(3575));
    pkt_connect_app->command_object->set("videoCodecs", Amf0Any::number(252));
    pkt_connect_app->command_object->set("videoFunction", Amf0Any::number(1));
    pkt_connect_app->command_object->set("pageUrl", Amf0Any::str());
    pkt_connect_app->command_object->set("objectEncoding", Amf0Any::number(0));

    // the debug_upnode is config in vhost and default to true.
    std::shared_ptr<Amf0Object> args = pkt_connect_app->args;

    // local ip of edge
    args->set("server_ip", Amf0Any::str("127.0.0.1"));
    if ((ret = protocol->send_packet(pkt_connect_app, 0)) != error_success) {
        tmss_error("send connect_app failed,ret={}", ret);
        return ret;
    }
    std::shared_ptr<RtmpSetWindowAckSizePacket> pkt_set_window_ack_size =
        std::make_shared<RtmpSetWindowAckSizePacket>();
    pkt_set_window_ack_size->ackowledgement_window_size = 2500000;
    if ((ret = protocol->send_packet(pkt_set_window_ack_size, 0)) != error_success) {
        tmss_error("set window ack size failed, ret={}", ret);
        return ret;
    }

    // receive connect response
    std::shared_ptr<RtmpConnectAppResPacket> pkt_response;
    if ((ret = protocol->expect_packet<RtmpConnectAppResPacket>(pkt_response)) != error_success) {
        tmss_error("receive connect app response failed, ret={}", ret);
        return ret;
    }

    std::shared_ptr<Amf0Any> data = pkt_response->info->get_property("data");
    if (data && data->is_ecma_array()) {
        std::shared_ptr<Amf0EcmaArray> arr = data->to_ecma_array();
        /*
        std::string primary;
        std::string authors;
        std::string version;
        std::string server_ip;
        std::string server;
        Amf0Any* prop = NULL;
        if ((prop = arr->ensure_property_string("primary")) != NULL) {
            primary = prop->to_str();
        }
        if ((prop = arr->ensure_property_string("authors")) != NULL) {
            authors = prop->to_str();
        }
        if ((prop = arr->ensure_property_string("version")) != NULL) {
            version = prop->to_str();
        }
        if ((prop = arr->ensure_property_string("server_ip")) != NULL) {
            server_ip = prop->to_str();
        }
        if ((prop = arr->ensure_property_string("server")) != NULL) {
            server = prop->to_str();
        }
        if ((prop = arr->ensure_property_number("id")) != NULL) {
            id = (int) prop->to_number();
        }
        if ((prop = arr->ensure_property_number("pid")) != NULL) {
            pid = (int) prop->to_number();
        }   //  */
    }
    return ret;
}

int RtmpClient::create_stream(int& stream_id) {
    int ret = error_success;
    // CreateStream
    std::shared_ptr<RtmpCreateStreamPacket> pkt_create_stream;
    if ((ret = protocol->send_packet(pkt_create_stream, 0)) != error_success) {
        tmss_error("send create_stream failed,ret={}", ret);
        return ret;
    }

    // CreateStream _result.
    std::shared_ptr<RtmpCreateStreamResPacket> pkt_response;
    if ((ret = protocol->expect_packet<RtmpCreateStreamResPacket>(pkt_response))
            != error_success) {
        tmss_error("expect create stream response message failed. ret={}",
                ret);
        return ret;
    }
    tmss_info("get create stream response message");

    stream_id = static_cast<int>(pkt_response->stream_id);

    return ret;
}

int RtmpClient::play(const std::string& stream, int stream_id) {
    int ret = error_success;

    // Play(stream)
    std::shared_ptr<RtmpPlayPacket> pkt_play = std::make_shared<RtmpPlayPacket>();
    pkt_play->stream_name = stream;
    if ((ret = protocol->send_packet(pkt_play, stream_id)) != error_success) {
        tmss_error("send play stream failed. "
            "stream={}, stream_id={}, ret={}",
            stream.c_str(), stream_id, ret);
        return ret;
    }

    // SetBufferLength(1000ms)
    int buffer_length_ms = 1000;
    std::shared_ptr<RtmpUserControlPacket> pkt_user_control =
        std::shared_ptr<RtmpUserControlPacket>();

    pkt_user_control->event_type = SrcPCUCSetBufferLength;
    pkt_user_control->event_data = stream_id;
    pkt_user_control->extra_data = buffer_length_ms;

    if ((ret = protocol->send_packet(pkt_user_control, 0)) != error_success) {
        tmss_error("send set buffer length failed. "
            "stream={}, stream_id={}, bufferLength={}, ret={}",
            stream.c_str(), stream_id, buffer_length_ms, ret);
        return ret;
    }

    // SetChunkSize
    std::shared_ptr<RtmpSetChunkSizePacket> pkt_set_chunk_size =
        std::shared_ptr<RtmpSetChunkSizePacket>();
    pkt_set_chunk_size->chunk_size = TMSS_CONSTS_RTMP_TMSS_CHUNK_SIZE;
    if ((ret = protocol->send_packet(pkt_set_chunk_size, 0)) != error_success) {
        tmss_error("send set chunk size failed. "
            "stream={}, chunk_size={}, ret={}",
            stream.c_str(), TMSS_CONSTS_RTMP_TMSS_CHUNK_SIZE, ret);
        return ret;
    }

    return ret;
}

int RtmpClient::publish(std::string stream, int stream_id) {
    int ret = error_success;

    // to do
    return ret;
}

int RtmpClient::fmle_publish(std::string stream, int& stream_id) {
    int ret = error_success;

    // to do
    return ret;
}

}   // namespace tmss
