/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#include <protocol/rtmp/rtmp_stack.hpp>

#include <utility>

#include <defs/err.hpp>
#include <log/log.hpp>
#include <util/util.hpp>

namespace tmss {
RtmpHandshakeBytes::RtmpHandshakeBytes() {
    c0c1 = s0s1s2 = c2 = NULL;
}

RtmpHandshakeBytes::~RtmpHandshakeBytes() {
    delete c0c1;
    delete s0s1s2;
    delete c2;
}

int RtmpHandshakeBytes::read_c0c1(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    if (c0c1) {
        return ret;
    }

    ssize_t nsize;

    c0c1 = new char[1537];
    if ((nsize = conn->read_fully(c0c1, 1537)) < 0) {
        tmss_warn("read c0c1 failed. ret={}", ret);
        return ret;
    }
    tmss_info("read c0c1 success.");

    return ret;
}

int RtmpHandshakeBytes::read_s0s1s2(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    if (s0s1s2) {
        return ret;
    }

    ssize_t nsize;

    s0s1s2 = new char[3073];
    if ((nsize = conn->read_fully(s0s1s2, 3073)) != error_success) {
        tmss_warn("read s0s1s2 failed. ret={}", ret);
        return ret;
    }
    tmss_info("read s0s1s2 success.");

    return ret;
}

int RtmpHandshakeBytes::read_c2(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    if (c2) {
        return ret;
    }

    ssize_t nsize;

    c2 = new char[1536];
    if ((nsize = conn->read_fully(c2, 1536)) != error_success) {
        tmss_warn("read c2 failed. ret={}", ret);
        return ret;
    }
    tmss_info("read c2 success.");

    return ret;
}

int RtmpHandshakeBytes::create_c0c1() {
    int ret = error_success;

    if (c0c1) {
        return ret;
    }

    c0c1 = new char[1537];
    random_generate(c0c1, 1537);

    // plain text required.
    Buffer buffer(c0c1, 9);
    buffer.write_1byte(0x03);
    buffer.write_4bytes(static_cast<int32_t>(::time(NULL)));
    buffer.write_4bytes(0x00);

    return ret;
}

int RtmpHandshakeBytes::create_s0s1s2(const char* c1) {
    int ret = error_success;

    if (s0s1s2) {
        return ret;
    }

    s0s1s2 = new char[3073];
    random_generate(s0s1s2, 3073);

    // plain text required.
    Buffer buffer(s0s1s2, 9);
    buffer.write_1byte(0x03);
    buffer.write_4bytes(static_cast<int32_t>(::time(NULL)));
    // s1 time2 copy from c1
    if (c0c1) {
        buffer.write_bytes(c0c1 + 1, 4);
    }

    // if c1 specified, copy c1 to s2.
    // @see: https://github.com/ossrs/srs/issues/46
    if (c1) {
        memcpy(s0s1s2 + 1537, c1, 1536);
    }

    return ret;
}

int RtmpHandshakeBytes::create_c2() {
    int ret = error_success;

    if (c2) {
        return ret;
    }

    c2 = new char[1536];
    random_generate(c2, 1536);

    // time
    Buffer buffer(c2, 8);
    buffer.write_4bytes(static_cast<int32_t>(::time(NULL)));
    // c2 time2 copy from s1
    if (s0s1s2) {
        buffer.write_bytes(s0s1s2 + 1, 4);
    }

    return ret;
}

RtmpPacket::RtmpPacket() {
}

int RtmpPacket::encode(int& size, char*& payload) {
    int ret = error_success;

    int wanted_size = get_size();
    char* wanted_payload = NULL;

    if (size > 0) {
        wanted_payload = new char[wanted_size];
    }

    Buffer stream(wanted_payload, wanted_size);
    if ((ret = encode_packet(&stream)) != error_success) {
        tmss_error("encode the packet failed. ret={}", ret);
        return ret;
    }

    size = wanted_size;
    payload = wanted_payload;
    tmss_info("encode the packet success. size={}", size);
}

int RtmpPacket::decode(Buffer* stream) {
    int ret = error_success;

    assert(stream != NULL);

    ret = error_rtmp_packet_invalid;
    tmss_error("current packet is not support to decode. ret={}", ret);

    return ret;
}

int RtmpPacket::get_prefer_cid() {
    return 0;
}

int RtmpPacket::get_message_type() {
    return 0;
}

int RtmpPacket::get_size() {
    return 0;
}

int RtmpPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    assert(stream != NULL);

    ret = error_rtmp_packet_invalid;
    tmss_error("current packet is not support to encode. ret={}", ret);

    return ret;
}

RtmpConnectAppPacket::RtmpConnectAppPacket() {
    command_name = RTMP_AMF0_COMMAND_CONNECT;
    transaction_id = 1;
    command_object = Amf0Any::object();
    // optional
    args = NULL;
}

RtmpConnectAppPacket::~RtmpConnectAppPacket() {
}

int RtmpConnectAppPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode connect command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CONNECT) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode connect command_name failed.command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode connect transaction_id failed. ret={}", ret);
        return ret;
    }

    // some client donot send id=1.0, so we only warn user if not match.
    if (transaction_id != 1.0) {
        ret = error_rtmp_amf0_decode;
        tmss_warn(
                "amf0 decode connect transaction_id failed.required=%.1f, actual=%.1f, ret={}",
                1.0, transaction_id, ret);
        ret = error_success;
    }

    if ((ret = command_object->read(stream)) != error_success) {
        tmss_error("amf0 decode connect command_object failed. ret={}", ret);
        return ret;
    }

    if (!stream->read_empty()) {
        // see: https://github.com/ossrs/srs/issues/186
        // the args maybe any amf0, for instance, a string. we should drop if not object.
        std::shared_ptr<Amf0Any> any;
        if ((ret = Amf0Any::discovery(stream, any)) != error_success) {
            tmss_error("amf0 find connect args failed. ret={}", ret);
            return ret;
        }
        assert(any);

        // read the instance
        if ((ret = any->read(stream)) != error_success) {
            tmss_error("amf0 decode connect args failed. ret={}", ret);
            return ret;
        }

        // drop when not an AMF0 object.
        if (!any->is_object()) {
            tmss_warn("drop the args, see: '4.1.1. connect', marker={}",
                    any->marker);
        } else {
            args = any->to_object();
        }
    }

    tmss_info("amf0 decode connect packet success");

    return ret;
}

int RtmpConnectAppPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpConnectAppPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpConnectAppPacket::get_size() {
    int size = 0;

    size += Amf0Size::str(command_name);
    size += Amf0Size::number();
    size += Amf0Size::object(command_object);
    if (args) {
        size += Amf0Size::object(args);
    }

    return size;
}

int RtmpConnectAppPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode ConnectAppPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode ConnectAppPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = command_object->write(stream)) != error_success) {
        tmss_error("encode ConnectAppPacket command_object failed. ret={}", ret);
        return ret;
    }

    if (args && (ret = args->write(stream)) != error_success) {
        tmss_error("encode ConnectAppPacket args failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode connect app request packet success.");

    return ret;
}

RtmpConnectAppResPacket::RtmpConnectAppResPacket() {
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = 1;
    props = Amf0Any::object();
    info = Amf0Any::object();
}

RtmpConnectAppResPacket::~RtmpConnectAppResPacket() {
}

int RtmpConnectAppResPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode connect command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode connect command_name failed. " "command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode connect transaction_id failed. ret={}", ret);
        return ret;
    }

    // some client donot send id=1.0, so we only warn user if not match.
    if (transaction_id != 1.0) {
        ret = error_rtmp_amf0_decode;
        tmss_warn(
                "amf0 decode connect transaction_id failed. " "required=%.1f, actual=%.1f, ret={}",
                1.0, transaction_id, ret);
        ret = error_success;
    }

    // for RED5(1.0.6), the props is NULL, we must ignore it.
    // @see https://github.com/ossrs/srs/issues/418
    if (!stream->read_empty()) {
        std::shared_ptr<Amf0Any> p;
        if ((ret = amf0_read_any(stream, p)) != error_success) {
            tmss_error("amf0 decode connect props failed. ret={}", ret);
            return ret;
        }

        // ignore when props is not amf0 object.
        if (!p->is_object()) {
            tmss_warn("ignore connect response props marker={}.",
                    static_cast<uint8_t>(p->marker));
            //  freep(p);
        } else {
            props = p->to_object();
            tmss_info("accept amf0 object connect response props");
        }
    }

    if ((ret = info->read(stream)) != error_success) {
        tmss_error("amf0 decode connect info failed. ret={}", ret);
        return ret;
    }

    tmss_info("amf0 decode connect response packet success");

    return ret;
}

int RtmpConnectAppResPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpConnectAppResPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpConnectAppResPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::object(props) + Amf0Size::object(info);
}

int RtmpConnectAppResPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode ConnectAppResPacket command_name failed. ret={}", ret);
        return ret;
    }
    tmss_info("encode command_name success.");

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode ConnectAppResPacket transaction_id failed. ret={}", ret);
        return ret;
    }
    tmss_info("encode transaction_id success.");

    if ((ret = props->write(stream)) != error_success) {
        tmss_error("encode ConnectAppResPacket props failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode props success.");

    if ((ret = info->write(stream)) != error_success) {
        tmss_error("encode ConnectAppResPacket info failed. ret={}", ret);
        return ret;
    }
    tmss_info("encode connect app response packet success.");

    return ret;
}

RtmpSetWindowAckSizePacket::RtmpSetWindowAckSizePacket() {
    ackowledgement_window_size = 0;
}

RtmpSetWindowAckSizePacket::~RtmpSetWindowAckSizePacket() {
}

int RtmpSetWindowAckSizePacket::decode(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(4)) {
        ret = error_rtmp_message_decode;
        tmss_error("decode ack window size failed. ret={}", ret);
        return ret;
    }

    ackowledgement_window_size = stream->read_4bytes();
    tmss_info("decode ack window size success");

    return ret;
}

int RtmpSetWindowAckSizePacket::get_prefer_cid() {
    return RTMP_CID_ProtocolControl;
}

int RtmpSetWindowAckSizePacket::get_message_type() {
    return RTMP_MSG_WindowAcknowledgementSize;
}

int RtmpSetWindowAckSizePacket::get_size() {
    return 4;
}

int RtmpSetWindowAckSizePacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(4)) {
        ret = error_rtmp_message_encode;
        tmss_error("encode ack size packet failed. ret={}", ret);
        return ret;
    }

    stream->write_4bytes(ackowledgement_window_size);

    tmss_info("encode ack size packet " "success. ack_size={}",
            ackowledgement_window_size);

    return ret;
}

RtmpPlayPacket::RtmpPlayPacket() {
    command_name = RTMP_AMF0_COMMAND_PLAY;
    transaction_id = 0;
    command_object = Amf0Any::null();

    start = -2;
    duration = -1;
    reset = true;
}

RtmpPlayPacket::~RtmpPlayPacket() {
    //  freep(command_object);
}

int RtmpPlayPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode play command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PLAY) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode play command_name failed. " "command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode play transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode play command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_string(stream, stream_name)) != error_success) {
        tmss_error("amf0 decode play stream_name failed. ret={}", ret);
        return ret;
    }

    if (!stream->read_empty() && (ret = amf0_read_number(stream, start)) != error_success) {
        tmss_error("amf0 decode play start failed. ret={}", ret);
        return ret;
    }
    if (!stream->read_empty() && (ret = amf0_read_number(stream, duration)) != error_success) {
        tmss_error("amf0 decode play duration failed. ret={}", ret);
        return ret;
    }

    if (stream->read_empty()) {
        return ret;
    }

    std::shared_ptr<Amf0Any> reset_value;
    if ((ret = amf0_read_any(stream, reset_value)) != error_success) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 read play reset marker failed. ret={}", ret);
        return ret;
    }
    //  SrsAutoFree(Amf0Any, reset_value);

    if (reset_value) {
        // check if the value is bool or number
        // An optional Boolean value or number that specifies whether
        // to flush any previous playlist
        if (reset_value->is_boolean()) {
            reset = reset_value->to_boolean();
        } else if (reset_value->is_number()) {
            reset = (reset_value->to_number() != 0);
        } else {
            ret = error_rtmp_amf0_decode;
            tmss_error("amf0 invalid type={}, requires number or bool, ret={}",
                    reset_value->marker, ret);
            return ret;
        }
    }

    tmss_info("amf0 decode play packet success");

    return ret;
}

int RtmpPlayPacket::get_prefer_cid() {
    return RTMP_CID_OverStream;
}

int RtmpPlayPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpPlayPacket::get_size() {
    int size = Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::str(stream_name);

    if (start != -2 || duration != -1 || !reset) {
        size += Amf0Size::number();
    }

    if (duration != -1 || !reset) {
        size += Amf0Size::number();
    }

    if (!reset) {
        size += Amf0Size::boolean();
    }

    return size;
}

int RtmpPlayPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode PlayPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode PlayPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode PlayPacket command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_string(stream, stream_name)) != error_success) {
        tmss_error("encode PlayPacket stream_name failed. ret={}", ret);
        return ret;
    }

    if ((start != -2 || duration != -1 || !reset) && (ret =
            amf0_write_number(stream, start)) != error_success) {
        tmss_error("encode PlayPacket start failed. ret={}", ret);
        return ret;
    }

    if ((duration != -1 || !reset)
            && (ret = amf0_write_number(stream, duration)) != error_success) {
        tmss_error("encode PlayPacket duration failed. ret={}", ret);
        return ret;
    }

    if (!reset && (ret = amf0_write_boolean(stream, reset)) != error_success) {
        tmss_error("encode PlayPacket reset failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode play request packet success.");

    return ret;
}

RtmpPlayResPacket::RtmpPlayResPacket() {
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = 0;
    command_object = Amf0Any::null();
    desc = Amf0Any::object();
}

RtmpPlayResPacket::~RtmpPlayResPacket() {
}

int RtmpPlayResPacket::get_prefer_cid() {
    return RTMP_CID_OverStream;
}

int RtmpPlayResPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpPlayResPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::object(desc);
}

int RtmpPlayResPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode PlayResPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode PlayResPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode PlayResPacket command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = desc->write(stream)) != error_success) {
        tmss_error("encode PlayResPacket desc failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode play response packet success.");

    return ret;
}

RtmpSetChunkSizePacket::RtmpSetChunkSizePacket() {
    chunk_size = TMSS_CONSTS_RTMP_PROTOCOL_CHUNK_SIZE;
}

RtmpSetChunkSizePacket::~RtmpSetChunkSizePacket() {
}

int RtmpSetChunkSizePacket::decode(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(4)) {
        ret = error_rtmp_message_decode;
        tmss_error("decode chunk size failed. ret={}", ret);
        return ret;
    }

    chunk_size = stream->read_4bytes();
    tmss_info("decode chunk size success. chunk_size={}", chunk_size);

    return ret;
}

int RtmpSetChunkSizePacket::get_prefer_cid() {
    return RTMP_CID_ProtocolControl;
}

int RtmpSetChunkSizePacket::get_message_type() {
    return RTMP_MSG_SetChunkSize;
}

int RtmpSetChunkSizePacket::get_size() {
    return 4;
}

int RtmpSetChunkSizePacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(4)) {
        ret = error_rtmp_message_encode;
        tmss_error("encode chunk packet failed. ret={}", ret);
        return ret;
    }

    stream->write_4bytes(chunk_size);

    tmss_info("encode chunk packet success. ack_size={}", chunk_size);

    return ret;
}

RtmpSetPeerBandwidthPacket::RtmpSetPeerBandwidthPacket() {
    bandwidth = 0;
    type = RtmpPeerBandwidthDynamic;
}

RtmpSetPeerBandwidthPacket::~RtmpSetPeerBandwidthPacket() {
}

int RtmpSetPeerBandwidthPacket::get_prefer_cid() {
    return RTMP_CID_ProtocolControl;
}

int RtmpSetPeerBandwidthPacket::get_message_type() {
    return RTMP_MSG_SetPeerBandwidth;
}

int RtmpSetPeerBandwidthPacket::get_size() {
    return 5;
}

int RtmpSetPeerBandwidthPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(5)) {
        ret = error_rtmp_message_encode;
        tmss_error("encode set bandwidth packet failed. ret={}", ret);
        return ret;
    }

    stream->write_4bytes(bandwidth);
    stream->write_1byte(type);

    tmss_info("encode set bandwidth packet " "success. bandwidth={}, type={}",
            bandwidth, type);

    return ret;
}

RtmpUserControlPacket::RtmpUserControlPacket() {
    event_type = 0;
    event_data = 0;
    extra_data = 0;
}

RtmpUserControlPacket::~RtmpUserControlPacket() {
}

int RtmpUserControlPacket::decode(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(2)) {
        ret = error_rtmp_message_decode;
        tmss_error("decode user control failed. ret={}", ret);
        return ret;
    }

    event_type = stream->read_2bytes();

    if (event_type == SrcPCUCFmsEvent0) {
        if (!stream->read_require(1)) {
            ret = error_rtmp_message_decode;
            tmss_error("decode user control failed. ret={}", ret);
            return ret;
        }
        event_data = stream->read_1byte();
    } else {
        if (!stream->read_require(4)) {
            ret = error_rtmp_message_decode;
            tmss_error("decode user control failed. ret={}", ret);
            return ret;
        }
        event_data = stream->read_4bytes();
    }

    if (event_type == SrcPCUCSetBufferLength) {
        if (!stream->read_require(4)) {
            ret = error_rtmp_message_encode;
            tmss_error("decode user control packet failed. ret={}", ret);
            return ret;
        }
        extra_data = stream->read_4bytes();
    }

    tmss_info(
            "decode user control success. " "event_type={}, event_data={}, extra_data={}",
            event_type, event_data, extra_data);

    return ret;
}

int RtmpUserControlPacket::get_prefer_cid() {
    return RTMP_CID_ProtocolControl;
}

int RtmpUserControlPacket::get_message_type() {
    return RTMP_MSG_UserControlMessage;
}

int RtmpUserControlPacket::get_size() {
    int size = 2;

    if (event_type == SrcPCUCFmsEvent0) {
        size += 1;
    } else {
        size += 4;
    }

    if (event_type == SrcPCUCSetBufferLength) {
        size += 4;
    }

    return size;
}

int RtmpUserControlPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(get_size())) {
        ret = error_rtmp_message_encode;
        tmss_error("encode user control packet failed. ret={}", ret);
        return ret;
    }

    stream->write_2bytes(event_type);

    if (event_type == SrcPCUCFmsEvent0) {
        stream->write_1byte(event_data);
    } else {
        stream->write_4bytes(event_data);
    }

    // when event type is set buffer length,
    // write the extra buffer length.
    if (event_type == SrcPCUCSetBufferLength) {
        stream->write_4bytes(extra_data);
        tmss_info("user control message, buffer_length={}", extra_data);
    }

    tmss_info(
            "encode user control packet success. " "event_type={}, event_data={}",
            event_type, event_data);

    return ret;
}

RtmpCreateStreamPacket::RtmpCreateStreamPacket() {
    command_name = RTMP_AMF0_COMMAND_CREATE_STREAM;
    transaction_id = 2;
    command_object = Amf0Any::null();
}

RtmpCreateStreamPacket::~RtmpCreateStreamPacket() {
}

int RtmpCreateStreamPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode createBuffer command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_CREATE_STREAM) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 decode createBuffer command_name failed. " "command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode createBuffer transaction_id failed. ret={}",
                ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode createBuffer command_object failed. ret={}",
                ret);
        return ret;
    }

    tmss_info("amf0 decode createBuffer packet success");

    return ret;
}

int RtmpCreateStreamPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpCreateStreamPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpCreateStreamPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null();
}

int RtmpCreateStreamPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode CreateStreamPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode CreateStreamPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode CreateStreamPacket command_object failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode create stream request packet success.");

    return ret;
}

RtmpCreateStreamResPacket::RtmpCreateStreamResPacket(double _transaction_id,
        double _stream_id) {
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = _transaction_id;
    command_object = Amf0Any::null();
    stream_id = _stream_id;
}

RtmpCreateStreamResPacket::~RtmpCreateStreamResPacket() {
}

int RtmpCreateStreamResPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode createBuffer command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode createBuffer command_name failed. " "command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode createBuffer transaction_id failed. ret={}",
                ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode createBuffer command_object failed. ret={}",
                ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, stream_id)) != error_success) {
        tmss_error("amf0 decode createBuffer stream_id failed. ret={}", ret);
        return ret;
    }

    tmss_info("amf0 decode createBuffer response packet success");

    return ret;
}

int RtmpCreateStreamResPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpCreateStreamResPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpCreateStreamResPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::number();
}

int RtmpCreateStreamResPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode CreateStreamResPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode CreateStreamResPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode CreateStreamResPacket command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, stream_id)) != error_success) {
        tmss_error("encode CreateStreamResPacket stream_id failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode createBuffer response packet success.");

    return ret;
}

RtmpFMLEStartPacket::RtmpFMLEStartPacket() {
    command_name = RTMP_AMF0_COMMAND_RELEASE_STREAM;
    transaction_id = 0;
    command_object = Amf0Any::null();
}

RtmpFMLEStartPacket::~RtmpFMLEStartPacket() {
}

int RtmpFMLEStartPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode FMLE start command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty()
            || (command_name != RTMP_AMF0_COMMAND_RELEASE_STREAM
                    && command_name != RTMP_AMF0_COMMAND_FC_PUBLISH
                    && command_name != RTMP_AMF0_COMMAND_UNPUBLISH)) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode FMLE start command_name failed.command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode FMLE start transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode FMLE start command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_string(stream, stream_name)) != error_success) {
        tmss_error("amf0 decode FMLE start stream_name failed. ret={}", ret);
        return ret;
    }

    tmss_info("amf0 decode FMLE start packet success");

    return ret;
}

int RtmpFMLEStartPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpFMLEStartPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpFMLEStartPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::str(stream_name);
}

int RtmpFMLEStartPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode FMLEStartPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode FMLEStartPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode FMLEStartPacket command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_string(stream, stream_name)) != error_success) {
        tmss_error("encode FMLEStartPacket stream_name failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode FMLE start response packet success.");

    return ret;
}

RtmpFMLEStartPacket* RtmpFMLEStartPacket::create_release_stream(std::string stream) {
    RtmpFMLEStartPacket* pkt = new RtmpFMLEStartPacket();

    pkt->command_name = RTMP_AMF0_COMMAND_RELEASE_STREAM;
    pkt->transaction_id = 2;
    pkt->stream_name = stream;

    return pkt;
}

RtmpFMLEStartPacket* RtmpFMLEStartPacket::create_FC_publish(std::string stream) {
    RtmpFMLEStartPacket* pkt = new RtmpFMLEStartPacket();

    pkt->command_name = RTMP_AMF0_COMMAND_FC_PUBLISH;
    pkt->transaction_id = 3;
    pkt->stream_name = stream;

    return pkt;
}

RtmpFMLEStartResPacket::RtmpFMLEStartResPacket(double _transaction_id) {
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = _transaction_id;
    command_object = Amf0Any::null();
    args = Amf0Any::undefined();
}

RtmpFMLEStartResPacket::~RtmpFMLEStartResPacket() {
}

int RtmpFMLEStartResPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode FMLE start response command_name failed. ret={}",
                ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_RESULT) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode FMLE start response command_name failed.command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error(
                "amf0 decode FMLE start response transaction_id failed. ret={}",
                ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error(
                "amf0 decode FMLE start response command_object failed. ret={}",
                ret);
        return ret;
    }

    if ((ret = amf0_read_undefined(stream)) != error_success) {
        tmss_error("amf0 decode FMLE start response stream_id failed. ret={}",
                ret);
        return ret;
    }

    tmss_info("amf0 decode FMLE start packet success");

    return ret;
}

int RtmpFMLEStartResPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpFMLEStartResPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpFMLEStartResPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::undefined();
}

int RtmpFMLEStartResPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode FMLEStartResPacket: command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode FMLEStartResPacket: transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode FMLEStartResPacket: command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_undefined(stream)) != error_success) {
        tmss_error("encode FMLEStartResPacket: args failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode FMLE start response packet success.");

    return ret;
}

RtmpPublishPacket::RtmpPublishPacket() {
    command_name = RTMP_AMF0_COMMAND_PUBLISH;
    transaction_id = 0;
    command_object = Amf0Any::null();
    type = "live";
}

RtmpPublishPacket::~RtmpPublishPacket() {
}

int RtmpPublishPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode publish command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PUBLISH) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode publish command_name failed.command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode publish transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode publish command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_string(stream, stream_name)) != error_success) {
        tmss_error("amf0 decode publish stream_name failed. ret={}", ret);
        return ret;
    }

    if (!stream->read_empty() && (ret = amf0_read_string(stream, type)) != error_success) {
        tmss_error("amf0 decode publish type failed. ret={}", ret);
        return ret;
    }

    tmss_info("amf0 decode publish packet success");

    return ret;
}

int RtmpPublishPacket::get_prefer_cid() {
    return RTMP_CID_OverStream;
}

int RtmpPublishPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpPublishPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::str(stream_name)
            + Amf0Size::str(type);
}

int RtmpPublishPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode PublishPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode PublishPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode PublishPacket command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_string(stream, stream_name)) != error_success) {
        tmss_error("encode PublishPacket stream_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_string(stream, type)) != error_success) {
        tmss_error("encode PublishPacket type failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode play request packet success.");

    return ret;
}

RtmpPausePacket::RtmpPausePacket() {
    command_name = RTMP_AMF0_COMMAND_PAUSE;
    transaction_id = 0;
    command_object = Amf0Any::null();

    time_ms = 0;
    is_pause = true;
}

RtmpPausePacket::~RtmpPausePacket() {
}

int RtmpPausePacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode pause command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_PAUSE) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode pause command_name failed. " "command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode pause transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode pause command_object failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_boolean(stream, is_pause)) != error_success) {
        tmss_error("amf0 decode pause is_pause failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, time_ms)) != error_success) {
        tmss_error("amf0 decode pause time_ms failed. ret={}", ret);
        return ret;
    }

    tmss_info("amf0 decode pause packet success");

    return ret;
}

RtmpOnStatusCallPacket::RtmpOnStatusCallPacket() {
    command_name = RTMP_AMF0_COMMAND_ON_STATUS;
    transaction_id = 0;
    args = Amf0Any::null();
    data = Amf0Any::object();
    on_status_str = "";
}

RtmpOnStatusCallPacket::~RtmpOnStatusCallPacket() {
}

int RtmpOnStatusCallPacket::decode(Buffer* stream) {
    int ret = error_success;

    // renyu: 解析检查onStatus命令字
    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode onStatus command_name failed. ret={}", ret);
        return ret;
    }

    if (command_name.empty() || command_name != RTMP_AMF0_COMMAND_ON_STATUS) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 onStatus command_name error. name={}", command_name.c_str());
        return ret;
    }

    // renyu: 解析检查transaction_id = 0
    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 onStatus transaction_id error. ret={}", ret);
        return ret;
    }

    if (transaction_id != 0) {
        ret = error_rtmp_amf0_decode;
        tmss_error("amf0 onStatus transaction_id error. id=%f", transaction_id);
        return ret;
    }

    // renyu: 解析检查null类型
    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 onStatus null error. ret={}", ret);
        return ret;
    }

    // renyu: 解析onStatus内容object
    if ((ret = data->read(stream)) != error_success) {
        tmss_error("amf0 onStatus object error. ret={}", ret);
        return ret;
    }

    std::shared_ptr<Amf0Any> status_content = data->get_property("code");
    if (status_content->is_string()) {
        on_status_str = status_content->to_str();
    }

    tmss_info("amf0 decode onStatus packet success");

    return ret;
}

int RtmpOnStatusCallPacket::get_prefer_cid() {
    return RTMP_CID_OverStream;
}

int RtmpOnStatusCallPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpOnStatusCallPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::object(data);
}

int RtmpOnStatusCallPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode OnStatusCallPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode OnStatusCallPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode OnStatusCallPacket args failed. ret={}", ret);
        return ret;
    }

    if ((ret = data->write(stream)) != error_success) {
        tmss_error("encode OnStatusCallPacket data failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode onStatus(Call) packet success.");

    return ret;
}

RtmpOnMetaDataPacket::RtmpOnMetaDataPacket() {
    name = TMSS_CONSTS_RTMP_ON_METADATA;
    metadata = Amf0Any::object();
}

RtmpOnMetaDataPacket::~RtmpOnMetaDataPacket() {
    //  freep(metadata);
}

int RtmpOnMetaDataPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, name)) != error_success) {
        tmss_error("decode metadata name failed. ret={}", ret);
        return ret;
    }

    // ignore the @setDataFrame
    if (name == TMSS_CONSTS_RTMP_SET_DATAFRAME) {
        if ((ret = amf0_read_string(stream, name)) != error_success) {
            tmss_error("decode metadata name failed. ret={}", ret);
            return ret;
        }
    }

    tmss_info("decode metadata name success. name={}", name.c_str());

    // the metadata maybe object or ecma array
    std::shared_ptr<Amf0Any> any;
    if ((ret = amf0_read_any(stream, any)) != error_success) {
        tmss_error("decode metadata metadata failed. ret={}", ret);
        return ret;
    }

    assert(any);
    if (any->is_object()) {
        metadata = any->to_object();
        tmss_info("decode metadata object success");
        return ret;
    }

    if (any->is_ecma_array()) {
        std::shared_ptr<Amf0EcmaArray> arr = any->to_ecma_array();

        // if ecma array, copy to object.
        for (int i = 0; i < arr->count(); i++) {
            metadata->set(arr->key_at(i), arr->value_at(i)->copy());
        }

        tmss_info("decode metadata array success");
    }

    return ret;
}

int RtmpOnMetaDataPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection2;
}

int RtmpOnMetaDataPacket::get_message_type() {
    return RTMP_MSG_AMF0DataMessage;
}

int RtmpOnMetaDataPacket::get_size() {
    return Amf0Size::str(name) + Amf0Size::object(metadata);
}

int RtmpOnMetaDataPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, name)) != error_success) {
        tmss_error("encode metadata name failed. ret={}", ret);
        return ret;
    }

    if ((ret = metadata->write(stream)) != error_success) {
        tmss_error("encode metadata failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode onMetaData packet success.");
    return ret;
}

void RtmpOnMetaDataPacket::fill_spaces(std::stringstream& ss, int level) {
    for (int i = 0; i < level; i++) {
        ss << "  ";
    }
}

void RtmpOnMetaDataPacket::get_body(std::shared_ptr<Amf0Any> any, std::stringstream& ss, int level) {
    if (any->is_boolean()) {
        ss << (any->to_boolean()? "true":"false") << std::endl;
    } else if (any->is_number()) {
        ss << any->to_number() << std::endl;
    } else if (any->is_string()) {
        ss << any->to_str_raw()<< std::endl;
    } else if (any->is_date()) {
        ss << std::hex << any->to_date() << "/" << std::hex << any->to_date_time_zone() << std::endl;
    } else if (any->is_null()) {
        ss << "Null" << std::endl;
    } else if (any->is_object()) {
        std::shared_ptr<Amf0Object> obj = any->to_object();
        for (int i = 0; i < obj->count(); i++) {
            fill_spaces(ss, level + 1);
            ss << obj->key_at(i) << ":";
            if (obj->value_at(i)->is_complex_object()) {
                get_body(obj->value_at(i), ss, level + 1);
            } else {
                get_body(obj->value_at(i), ss, 0);
            }
        }
    } else if (any->is_ecma_array()) {
        std::shared_ptr<Amf0EcmaArray> obj = any->to_ecma_array();
        for (int i = 0; i < obj->count(); i++) {
            fill_spaces(ss, level + 1);
            ss << obj->key_at(i) << ":";
            if (obj->value_at(i)->is_complex_object()) {
                get_body(obj->value_at(i), ss, level + 1);
            } else {
                get_body(obj->value_at(i), ss, 0);
            }
        }
    } else if (any->is_strict_array()) {
        std::shared_ptr<Amf0StrictArray> obj = any->to_strict_array();
        for (int i = 0; i < obj->count(); i++) {
            fill_spaces(ss, level + 1);
            if (obj->at(i)->is_complex_object())  {
                get_body(obj->at(i), ss, level + 1);
            } else {
                get_body(obj->at(i), ss, 0);
            }
        }
    } else {
        ss << "Unknown" << std::endl;
    }
}

RtmpBandwidthPacket::RtmpBandwidthPacket() {
    command_name = RTMP_AMF0_COMMAND_ON_STATUS;
    transaction_id = 0;
    args = Amf0Any::null();
    data = Amf0Any::object();
}

RtmpBandwidthPacket::~RtmpBandwidthPacket() {
}

int RtmpBandwidthPacket::decode(Buffer *stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode bwtc command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode bwtc transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode bwtc command_object failed. ret={}", ret);
        return ret;
    }

    // @remark, for bandwidth test, ignore the data field.
    // only decode the stop-play, start-publish and finish packet.
    if (is_stop_play() || is_start_publish() || is_finish()) {
        if ((ret = data->read(stream)) != error_success) {
            tmss_error("amf0 decode bwtc command_object failed. ret={}", ret);
            return ret;
        }
    }

    tmss_info("decode RtmpBandwidthPacket success.");

    return ret;
}

int RtmpBandwidthPacket::get_prefer_cid() {
    return RTMP_CID_OverStream;
}

int RtmpBandwidthPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpBandwidthPacket::get_size() {
    return Amf0Size::str(command_name) + Amf0Size::number()
            + Amf0Size::null() + Amf0Size::object(data);
}

int RtmpBandwidthPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode BandwidthPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode BandwidthPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_null(stream)) != error_success) {
        tmss_error("encode BandwidthPacket args failed. ret={}", ret);
        return ret;
    }

    if ((ret = data->write(stream)) != error_success) {
        tmss_error("encode BandwidthPacket data failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode onStatus(Call) packet success.");

    return ret;
}

bool RtmpBandwidthPacket::is_start_play() {
    return command_name == TMSS_BW_CHECK_START_PLAY;
}

bool RtmpBandwidthPacket::is_starting_play() {
    return command_name == TMSS_BW_CHECK_STARTING_PLAY;
}

bool RtmpBandwidthPacket::is_stop_play() {
    return command_name == TMSS_BW_CHECK_STOP_PLAY;
}

bool RtmpBandwidthPacket::is_stopped_play() {
    return command_name == TMSS_BW_CHECK_STOPPED_PLAY;
}

bool RtmpBandwidthPacket::is_start_publish() {
    return command_name == TMSS_BW_CHECK_START_PUBLISH;
}

bool RtmpBandwidthPacket::is_starting_publish() {
    return command_name == TMSS_BW_CHECK_STARTING_PUBLISH;
}

bool RtmpBandwidthPacket::is_stop_publish() {
    return command_name == TMSS_BW_CHECK_STOP_PUBLISH;
}

bool RtmpBandwidthPacket::is_stopped_publish() {
    return command_name == TMSS_BW_CHECK_STOPPED_PUBLISH;
}

bool RtmpBandwidthPacket::is_finish() {
    return command_name == TMSS_BW_CHECK_FINISHED;
}

bool RtmpBandwidthPacket::is_final() {
    return command_name == TMSS_BW_CHECK_FINAL;
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_start_play() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_START_PLAY);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_starting_play() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_STARTING_PLAY);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_playing() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_PLAYING);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_stop_play() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_STOP_PLAY);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_stopped_play() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_STOPPED_PLAY);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_start_publish() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_START_PUBLISH);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_starting_publish() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_STARTING_PUBLISH);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_publishing() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_PUBLISHING);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_stop_publish() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_STOP_PUBLISH);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_stopped_publish() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_STOPPED_PUBLISH);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_finish() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_FINISHED);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::create_final() {
    RtmpBandwidthPacket* pkt = new RtmpBandwidthPacket();
    return pkt->set_command(TMSS_BW_CHECK_FINAL);
}

RtmpBandwidthPacket* RtmpBandwidthPacket::set_command(std::string command) {
    command_name = command;

    return this;
}

RtmpCloseStreamPacket::RtmpCloseStreamPacket() {
    command_name = RTMP_AMF0_COMMAND_CLOSE_STREAM;
    transaction_id = 0;
    command_object = Amf0Any::null();
}

RtmpCloseStreamPacket::~RtmpCloseStreamPacket() {
}

int RtmpCloseStreamPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode closeBuffer command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode closeBuffer transaction_id failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_read_null(stream)) != error_success) {
        tmss_error("amf0 decode closeBuffer command_object failed. ret={}", ret);
        return ret;
    }
    tmss_info("amf0 decode closeBuffer packet success");

    return ret;
}

RtmpCallPacket::RtmpCallPacket() {
    command_name = "";
    transaction_id = 0;
    command_object = NULL;
    arguments = NULL;
}

RtmpCallPacket::~RtmpCallPacket() {
}

int RtmpCallPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, command_name)) != error_success) {
        tmss_error("amf0 decode call command_name failed. ret={}", ret);
        return ret;
    }
    if (command_name.empty()) {
        ret = error_rtmp_amf0_decode;
        tmss_error(
                "amf0 decode call command_name failed. " "command_name={}, ret={}",
                command_name.c_str(), ret);
        return ret;
    }

    if ((ret = amf0_read_number(stream, transaction_id)) != error_success) {
        tmss_error("amf0 decode call transaction_id failed. ret={}", ret);
        return ret;
    }
    if ((ret = Amf0Any::discovery(stream, command_object)) != error_success) {
        tmss_error("amf0 discovery call command_object failed. ret={}", ret);
        return ret;
    }
    if ((ret = command_object->read(stream)) != error_success) {
        tmss_error("amf0 decode call command_object failed. ret={}", ret);
        return ret;
    }

    if (!stream->read_empty()) {
        if ((ret = Amf0Any::discovery(stream, arguments)) != error_success) {
            tmss_error("amf0 discovery call arguments failed. ret={}", ret);
            return ret;
        }
        if ((ret = arguments->read(stream)) != error_success) {
            tmss_error("amf0 decode call arguments failed. ret={}", ret);
            return ret;
        }
    }

    tmss_info("amf0 decode call packet success");

    return ret;
}

int RtmpCallPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpCallPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpCallPacket::get_size() {
    int size = 0;

    size += Amf0Size::str(command_name) + Amf0Size::number();

    if (command_object) {
        size += command_object->total_size();
    }

    if (arguments) {
        size += arguments->total_size();
    }

    return size;
}

int RtmpCallPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode CallPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode CallPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if (command_object && (ret = command_object->write(stream)) != error_success) {
        tmss_error("encode CallPacket command_object failed. ret={}", ret);
        return ret;
    }

    if (arguments && (ret = arguments->write(stream)) != error_success) {
        tmss_error("encode CallPacket arguments failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode create stream request packet success.");

    return ret;
}

RtmpCallResPacket::RtmpCallResPacket(double _transaction_id) {
    command_name = RTMP_AMF0_COMMAND_RESULT;
    transaction_id = _transaction_id;
    command_object = NULL;
    response = NULL;
}

RtmpCallResPacket::~RtmpCallResPacket() {
}

int RtmpCallResPacket::get_prefer_cid() {
    return RTMP_CID_OverConnection;
}

int RtmpCallResPacket::get_message_type() {
    return RTMP_MSG_AMF0CommandMessage;
}

int RtmpCallResPacket::get_size() {
    int size = 0;

    size += Amf0Size::str(command_name) + Amf0Size::number();

    if (command_object) {
        size += command_object->total_size();
    }

    if (response) {
        size += response->total_size();
    }

    return size;
}

int RtmpCallResPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_write_string(stream, command_name)) != error_success) {
        tmss_error("encode CallResPacket command_name failed. ret={}", ret);
        return ret;
    }

    if ((ret = amf0_write_number(stream, transaction_id)) != error_success) {
        tmss_error("encode CallResPacket transaction_id failed. ret={}", ret);
        return ret;
    }

    if (command_object && (ret = command_object->write(stream)) != error_success) {
        tmss_error("encode CallResPacket command_object failed. ret={}", ret);
        return ret;
    }

    if (response && (ret = response->write(stream)) != error_success) {
        tmss_error("encode CallResPacket response failed. ret={}", ret);
        return ret;
    }

    tmss_info("encode call response packet success.");

    return ret;
}

RtmpOnCustomAmfDataPacket::RtmpOnCustomAmfDataPacket() {
    name = "";
}

RtmpOnCustomAmfDataPacket::~RtmpOnCustomAmfDataPacket() {
}

// renyu: 解析仅解析name
int RtmpOnCustomAmfDataPacket::decode(Buffer* stream) {
    int ret = error_success;

    if ((ret = amf0_read_string(stream, name)) != error_success) {
        tmss_error("decode custom amf data name failed. ret={}", ret);
        return ret;
    }

    tmss_info("decode custom amf data name success. name={}", name.c_str());

    return ret;
}

RtmpAcknowledgementPacket::RtmpAcknowledgementPacket() {
    sequence_number = 0;
}

RtmpAcknowledgementPacket::~RtmpAcknowledgementPacket() {
}

int RtmpAcknowledgementPacket::decode(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(4)) {
        ret = error_rtmp_message_decode;
        tmss_error("decode acknowledgement failed. ret={}", ret);
        return ret;
    }

    sequence_number = (uint32_t) stream->read_4bytes();
    tmss_info("decode acknowledgement success");

    return ret;
}

int RtmpAcknowledgementPacket::get_prefer_cid() {
    return RTMP_CID_ProtocolControl;
}

int RtmpAcknowledgementPacket::get_message_type() {
    return RTMP_MSG_Acknowledgement;
}

int RtmpAcknowledgementPacket::get_size() {
    return 4;
}

int RtmpAcknowledgementPacket::encode_packet(Buffer* stream) {
    int ret = error_success;

    if (!stream->read_require(4)) {
        ret = error_rtmp_message_encode;
        tmss_error("encode acknowledgement packet failed. ret={}", ret);
        return ret;
    }

    stream->write_4bytes(sequence_number);

    tmss_info("encode acknowledgement packet " "success. sequence_number={}",
            sequence_number);

    return ret;
}

AckWindowSize::AckWindowSize() {
    window = 0;
    sequence_number = nb_recv_bytes = 0;
}

RtmpProtocolHandler::RtmpProtocolHandler(std::shared_ptr<IConn> io, bool fix_timestamp) {
    this->conn = io;
    this->fix_rtmp_timestamp = fix_timestamp;

    for (int cid = 0; cid < TMSS_PERF_CHUNK_STREAM_CACHE; cid++) {
        std::shared_ptr<RtmpChunkStream> cs = std::make_shared<RtmpChunkStream>(cid);
        // set the perfer cid of chunk,
        // which will copy to the message received.
        cs->header.perfer_cid = cid;

        cs_cache.push_back(cs);
    }
}

RtmpProtocolHandler::~RtmpProtocolHandler() {
}

int RtmpProtocolHandler::send_packet(std::shared_ptr<RtmpPacket> pkt, int streamid) {
    int ret = error_success;

    int size = 0;
    char* payload = NULL;
    if ((ret = pkt->encode(size, payload)) != error_success) {
        tmss_error(
                "encode RTMP packet to bytes oriented RTMP message failed. ret={}",
                ret);
        return ret;
    }

    // encode packet to payload and size.
    if (size <= 0 || payload == NULL) {
        tmss_warn("packet is empty, ignore empty message.");
        return ret;
    }

    // to message
    MessageHeader header;
    header.payload_length = size;
    header.message_type = pkt->get_message_type();
    header.stream_id = streamid;
    header.perfer_cid = pkt->get_prefer_cid();

    // we directly send out the packet,
    // use very simple algorithm, not very fast,
    // but it's ok.
    char* p = payload;
    char* end = p + size;
    char c0c3[TMSS_CONSTS_RTMP_MAX_FMT0_HEADER_SIZE];
    while (p < end) {
        int nbh = 0;
        if (p == payload) {
            nbh = chunk_header_c0(header.perfer_cid, header.timestamp,
                    header.payload_length, header.message_type, header.stream_id, c0c3,
                    sizeof(c0c3));
        } else {
            nbh = chunk_header_c3(header.perfer_cid, header.timestamp, c0c3,
                    sizeof(c0c3));
        }

        iovec iovs[2];
        iovs[0].iov_base = c0c3;
        iovs[0].iov_len = nbh;

        int payload_size = Min(end - p, out_chunk_size);
        iovs[1].iov_base = p;
        iovs[1].iov_len = payload_size;
        p += payload_size;

        if ((ret = conn->writev(iovs, 2)) != error_success) {
            tmss_error("send packet with writev failed. ret={}", ret);
            return ret;
        }
    }

    // ignore raw bytes oriented RTMP message.
    if (pkt == nullptr) {
        return ret;
    }

    switch (header.message_type) {
        case RTMP_MSG_SetChunkSize: {
            auto packet =
                    std::dynamic_pointer_cast<RtmpSetChunkSizePacket>(pkt);
            out_chunk_size = packet->chunk_size;
            tmss_info("out.chunk={}", packet->chunk_size);
            break;
        }
        case RTMP_MSG_WindowAcknowledgementSize: {
            auto packet =
                    std::dynamic_pointer_cast<RtmpSetWindowAckSizePacket>(pkt);
            out_ack_size.window = static_cast<uint32_t>(packet->ackowledgement_window_size);
            break;
        }
        case RTMP_MSG_AMF0CommandMessage:
        case RTMP_MSG_AMF3CommandMessage: {
            if (true) {
                auto packet =
                        std::dynamic_pointer_cast<RtmpConnectAppPacket>(pkt);
                if (packet) {
                    requests[packet->transaction_id] = packet->command_name;
                    break;
                }
            }
            if (true) {
                auto packet =
                        std::dynamic_pointer_cast<RtmpCreateStreamPacket>(pkt);
                if (packet) {
                    requests[packet->transaction_id] = packet->command_name;
                    break;
                }
            }
            if (true) {
                auto packet = std::dynamic_pointer_cast<RtmpFMLEStartPacket>(pkt);
                if (packet) {
                    requests[packet->transaction_id] = packet->command_name;
                    break;
                }
            }
            break;
        }
        case RTMP_MSG_VideoMessage:
        case RTMP_MSG_AudioMessage:
        default:
            break;
    }

    return ret;
}

int RtmpProtocolHandler::recv_packet(std::shared_ptr<RtmpPacket>& pkt) {
    int ret = error_success;

    while (true) {
        std::shared_ptr<RtmpMessage> msg;

        if ((ret = recv_interlaced_message(msg)) != error_success) {
            tmss_info("recv interlaced message failed. ret={}", ret);
            if (ret != error_socket_timeout) {
                tmss_error("recv interlaced message failed. ret={}", ret);
            }
            return ret;
        }
        tmss_info("entire msg received");

        if (!msg) {
            tmss_info("got empty message without error.");
            continue;
        }

        if (msg->size <= 0 || msg->header.payload_length <= 0) {
            tmss_info("ignore empty message(type={}, size={}, time={}, sid={}).",
                    msg->header.message_type, msg->header.payload_length,
                    msg->header.timestamp, msg->header.stream_id);
            continue;
        }

        if ((ret = on_recv_message(msg, pkt)) != error_success) {
            tmss_error("hook the received msg failed. ret={}", ret);
            return ret;
        }

        tmss_info("got a msg, cid={}, type={}, size={}, time={}",
                msg->header.perfer_cid, msg->header.message_type,
                msg->header.payload_length, msg->header.timestamp);
        //  *pmsg = msg;
        break;
    }
    return ret;
}

int RtmpProtocolHandler::recv_message(char* buf, int size) {
    int ret = error_success;

    while (true) {
        std::shared_ptr<RtmpMessage> msg;

        if ((ret = recv_interlaced_message(msg, buf, size)) != error_success) {
            tmss_info("recv interlaced message failed. ret={}", ret);
            if (ret != error_socket_timeout) {
                tmss_error("recv interlaced message failed. ret={}", ret);
            }
            return ret;
        }
        tmss_info("entire msg received");

        if (!msg) {
            tmss_info("got empty message without error.");
            continue;
        }

        if (msg->size <= 0 || msg->header.payload_length <= 0) {
            tmss_info("ignore empty message(type={}, size={}, time={}, sid={}).",
                    msg->header.message_type, msg->header.payload_length,
                    msg->header.timestamp, msg->header.stream_id);
            continue;
        }

        std::shared_ptr<RtmpPacket> pkt;
        if ((ret = on_recv_message(msg, pkt)) != error_success) {
            tmss_error("hook the received msg failed. ret={}", ret);
            return ret;
        }

        tmss_info("got a msg, cid={}, type={}, size={}, time={}",
                msg->header.perfer_cid, msg->header.message_type,
                msg->header.payload_length, msg->header.timestamp);

        ret = msg->size;
        break;
    }
    return ret;
}

int RtmpProtocolHandler::send_message(const char* buf, int size) {
    int ret = error_success;
    return ret;
}

int RtmpProtocolHandler::recv_interlaced_message(std::shared_ptr<RtmpMessage>& msg,
        char* recv_buf, int buf_size) {
    int ret = error_success;

    // chunk stream basic header.
    char fmt = 0;
    int cid = 0;
    if ((ret = read_basic_header(fmt, cid)) != error_success) {
        if (ret != error_socket_timeout) {
                //  && !srs_is_client_gracefully_close(ret)) {
            tmss_error("read basic header failed. ret={}", ret);
        }
        return ret;
    }
    tmss_info("read basic header success. fmt={}, cid={}", fmt, cid);

    // the cid must not negative.
    assert(cid >= 0);

    // get the cached chunk stream.
    std::shared_ptr<RtmpChunkStream> chunk;  //  = std::make_shared<RtmpChunkStream>();

    // use chunk stream cache to get the chunk info.
    // @see https://github.com/ossrs/srs/issues/249
    if (cid < TMSS_PERF_CHUNK_STREAM_CACHE) {
        // chunk stream cache hit.
        tmss_info("cs-cache hit, cid={}", cid);
        // already init, use it direclty
        chunk = cs_cache[cid];
        tmss_info(
                "cached chunk stream: fmt={}, cid={}, size={}, message(type={}, size={}, time={}, sid={})",
                chunk->fmt, chunk->cid, (chunk->msg? chunk->msg->size : 0),
                chunk->header.message_type, chunk->header.payload_length,
                chunk->header.timestamp, chunk->header.stream_id);
    } else {
        // chunk stream cache miss, use map.
        if (chunk_streams.find(cid) == chunk_streams.end()) {
            chunk = chunk_streams[cid] = std::make_shared<RtmpChunkStream>(cid);
            // set the perfer cid of chunk,
            // which will copy to the message received.
            chunk->header.perfer_cid = cid;
            tmss_info("cache new chunk stream: fmt={}, cid={}", fmt, cid);
        } else {
            chunk = chunk_streams[cid];
            tmss_info(
                    "cached chunk stream: fmt={}, cid={}, size={}, message(type={}, size={}, time={}, sid={})",
                    chunk->fmt, chunk->cid, (chunk->msg? chunk->msg->size : 0),
                    chunk->header.message_type, chunk->header.payload_length,
                    chunk->header.timestamp, chunk->header.stream_id);
        }
    }

    // chunk stream message header
    if ((ret = read_message_header(chunk, fmt)) != error_success) {
        if (ret != error_socket_timeout) {
                //  && !srs_is_client_gracefully_close(ret)) {
            tmss_error("read message header failed. ret={}", ret);
        }
        return ret;
    }
    tmss_info("read message header success. "
            "fmt={}, ext_time={}, size={}, message(type={}, size={}, time={}, sid={})",
            fmt, chunk->extended_timestamp, (chunk->msg? chunk->msg->size : 0),
            chunk->header.message_type, chunk->header.payload_length,
            chunk->header.timestamp, chunk->header.stream_id);

    // read msg payload from chunk stream.
    //  RtmpMessage* msg = NULL;
    if (recv_buf && (buf_size > 0)) {
        chunk->msg->set_payload(recv_buf);
        if (buf_size < chunk->header.payload_length) {
            ret = error_buffer_not_enough;
            tmss_info("buffer may be too small. buf_size={},payload_length={},ret={}",
                buf_size, chunk->header.payload_length, ret);
            return ret;
        }
    }
    if ((ret = read_message_payload(chunk, msg)) != error_success) {
        tmss_info("read message payload failed. ret={}", ret);
        if (ret != error_socket_timeout) {
                //  && !srs_is_client_gracefully_close(ret)) {
            tmss_error("read message payload failed. ret={}", ret);
        }
        return ret;
    }

    // not got an entire RTMP message, try next chunk.
    if (!msg) {
        tmss_info(
                "get partial message success. size={}, message(type={}, size={}, time={}, sid={})",
                (msg? msg->size : (chunk->msg? chunk->msg->size : 0)),
                chunk->header.message_type, chunk->header.payload_length,
                chunk->header.timestamp, chunk->header.stream_id);
        return ret;
    }

    //  msg->header.proto_type = TMSS_MSG_PROTO_RTMP;
    //  pmsg = msg;
    tmss_info("get entire message success. size={}, message(type={}, size={}, time={}, sid={})",
            (msg ? msg->size : (chunk->msg ? chunk->msg->size : 0)),
            chunk->header.message_type, chunk->header.payload_length,
            chunk->header.timestamp, chunk->header.stream_id);

    return ret;
}

/**
 * 6.1.1. Chunk Basic Header
 * The Chunk Basic Header encodes the chunk stream ID and the chunk
 * type(represented by fmt field in the figure below). Chunk type
 * determines the format of the encoded message header. Chunk Basic
 * Header field may be 1, 2, or 3 bytes, depending on the chunk stream
 * ID.
 *
 * The bits 0-5 (least significant) in the chunk basic header represent
 * the chunk stream ID.
 *
 * Chunk stream IDs 2-63 can be encoded in the 1-byte version of this
 * field.
 *    0 1 2 3 4 5 6 7
 *   +-+-+-+-+-+-+-+-+
 *   |fmt|   cs id   |
 *   +-+-+-+-+-+-+-+-+
 *   Figure 6 Chunk basic header 1
 *
 * Chunk stream IDs 64-319 can be encoded in the 2-byte version of this
 * field. ID is computed as (the second byte + 64).
 *   0                   1
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |fmt|    0      | cs id - 64    |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   Figure 7 Chunk basic header 2
 *
 * Chunk stream IDs 64-65599 can be encoded in the 3-byte version of
 * this field. ID is computed as ((the third byte)*256 + the second byte
 * + 64).
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |fmt|     1     |         cs id - 64            |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   Figure 8 Chunk basic header 3
 *
 * cs id: 6 bits
 * fmt: 2 bits
 * cs id - 64: 8 or 16 bits
 *
 * Chunk stream IDs with values 64-319 could be represented by both 2-
 * byte version and 3-byte version of this field.
 */
int RtmpProtocolHandler::read_basic_header(char& fmt, int& cid) {
    int ret = error_success;

    char basic_header[4] = {0};
    char* pos = basic_header;
    // if ((ret = in_buffer->grow(skt, 1)) != error_success) {
    if ((ret = io_buffer->read_bytes(pos, 1)) != error_success) {
        if (ret != error_socket_timeout) {
            tmss_error("read 1bytes basic header failed. required_size={}, ret={}",
                    1, ret);
        }
        return ret;
    }

    fmt = *pos++;
    cid = fmt & 0x3f;
    fmt = (fmt >> 6) & 0x03;

    // 2-63, 1B chunk header
    if (cid > 1) {
        tmss_info("basic header parsed. fmt={}, cid={}", fmt, cid);
        return ret;
    }

    // 64-319, 2B chunk header
    if (cid == 0) {
        if ((ret = io_buffer->read_bytes(pos, 1)) != error_success) {
            if (ret != error_socket_timeout) {
                tmss_error(
                        "read 2bytes basic header failed. required_size={}, ret={}",
                        1, ret);
            }
            return ret;
        }

        cid = 64;
        cid += static_cast<uint8_t>(*pos++);
        tmss_info("2bytes basic header parsed. fmt={}, cid={}", fmt, cid);
        // 64-65599, 3B chunk header
    } else if (cid == 1) {
        if ((ret = io_buffer->read_bytes(pos, 2)) != error_success) {
            if (ret != error_socket_timeout) {
                tmss_error(
                        "read 3bytes basic header failed. required_size={}, ret={}",
                        2, ret);
            }
            return ret;
        }

        cid = 64;
        cid += static_cast<uint8_t>(*pos++);
        cid += (static_cast<uint8_t>(*pos++)) * 256;
        tmss_info("3bytes basic header parsed. fmt={}, cid={}", fmt, cid);
    } else {
        tmss_error("invalid path, impossible basic header.");
        assert(false);
    }

    return ret;
}

/**
 * parse the message header.
 *   3bytes: timestamp delta,    fmt=0,1,2
 *   3bytes: payload length,     fmt=0,1
 *   1bytes: message type,       fmt=0,1
 *   4bytes: stream id,          fmt=0
 * where:
 *   fmt=0, 0x0X
 *   fmt=1, 0x4X
 *   fmt=2, 0x8X
 *   fmt=3, 0xCX
 */
int RtmpProtocolHandler::read_message_header(std::shared_ptr<RtmpChunkStream> chunk, char fmt) {
    int ret = error_success;
    // fresh packet used to update the timestamp even fmt=3 for first packet.
    // fresh packet always means the chunk is the first one of message.
    bool is_first_chunk_of_msg = !chunk->msg;

    // but, we can ensure that when a chunk stream is fresh,
    // the fmt must be 0, a new stream.
    if (chunk->msg_count == 0 && fmt != RTMP_FMT_TYPE0) {
        if (fmt == RTMP_FMT_TYPE1) {
            tmss_warn("accept cid=2, fmt=1 to make librtmp happy.");
        } else {
            // must be a RTMP protocol level error.
            ret = error_rtmp_chunk_start;
            tmss_error(
                    "chunk stream is fresh, fmt must be {}, actual is {}. cid={}, ret={}",
                    RTMP_FMT_TYPE0, fmt, chunk->cid, ret);
            return ret;
        }
    }

    // when exists cache msg, means got an partial message,
    // the fmt must not be type0 which means new message.
    if (chunk->msg && fmt == RTMP_FMT_TYPE0) {
        ret = error_rtmp_chunk_start;
        tmss_error(
                "chunk stream exists, " "fmt must not be {}, actual is {}. ret={}",
                RTMP_FMT_TYPE0, fmt, ret);
        return ret;
    }

    // create msg when new chunk stream start
    if (!chunk->msg) {
        chunk->msg = std::make_shared<RtmpMessage>();
        tmss_info("create message for new chunk, fmt={}, cid={}", fmt,
                chunk->cid);
    }

    // read message header from socket to buffer.
    static char mh_sizes[] = { 11, 7, 3, 0 };
    int mh_size = mh_sizes[static_cast<int>(fmt)];
    tmss_info("calc chunk message header size. fmt={}, mh_size={}", fmt,
            mh_size);
    std::shared_ptr<char> data(new char[mh_size]);
    char *pos = data.get();
    if (mh_size > 0 && (ret = io_buffer->read_bytes(pos, mh_size)) != error_success) {
        if (ret != error_socket_timeout) {
            tmss_error("read {}bytes message header failed. ret={}", mh_size,
                    ret);
        }
        return ret;
    }

    /**
     * parse the message header.
     *   3bytes: timestamp delta,    fmt=0,1,2
     *   3bytes: payload length,     fmt=0,1
     *   1bytes: message type,       fmt=0,1
     *   4bytes: stream id,          fmt=0
     * where:
     *   fmt=0, 0x0X
     *   fmt=1, 0x4X
     *   fmt=2, 0x8X
     *   fmt=3, 0xCX
     */
    // see also: ngx_rtmp_recv
    if (fmt <= RTMP_FMT_TYPE2) {
        char* p = pos;
        pos += mh_size;

        char* pp = reinterpret_cast<char*>(&chunk->header.timestamp_delta);
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;
        pp[3] = 0;

        // fmt: 0
        // timestamp: 3 bytes
        // fmt: 1 or 2
        // timestamp delta: 3 bytes
        chunk->extended_timestamp = (chunk->header.timestamp_delta
                >= RTMP_EXTENDED_TIMESTAMP);
        if (!chunk->extended_timestamp) {
            // Extended timestamp: 0 or 4 bytes
            if (fmt == RTMP_FMT_TYPE0) {
                //  6.1.2.1. Type 0
                //  For a type-0 chunk, the absolute timestamp of the message is sent
                //  here.
                chunk->header.timestamp = chunk->header.timestamp_delta;
                chunk->header.timestamp_delta = 0;  //  if next fmt = 3, the delta = 0
            } else {
                //  6.1.2.2. Type 1
                //  6.1.2.3. Type 2
                //  For a type-1 or type-2 chunk, the difference between the previous
                //  chunk's timestamp and the current chunk's timestamp is sent here.
                chunk->header.timestamp += chunk->header.timestamp_delta;
            }
            chunk->extend_time = 0;
            tmss_info("no extend timestamp fmt:{}, timestamp:{}, delta:{}",
                fmt, chunk->header.timestamp, chunk->header.timestamp_delta);
        }

        if (fmt <= RTMP_FMT_TYPE1) {
            int32_t payload_length = 0;
            pp = reinterpret_cast<char*>(&payload_length);
            pp[2] = *p++;
            pp[1] = *p++;
            pp[0] = *p++;
            pp[3] = 0;

            // for a message, if msg exists in cache, the size must not changed.
            // always use the actual msg size to compare, for the cache payload length can changed,
            // for the fmt type1(stream_id not changed), user can change the payload
            // length(it's not allowed in the continue chunks).
            if (!is_first_chunk_of_msg
                    && chunk->header.payload_length != payload_length) {
                ret = error_rtmp_packet_size;
                tmss_error(
                        "msg exists in chunk cache, " "size={} cannot change to {}, ret={}",
                        chunk->header.payload_length, payload_length, ret);
                return ret;
            }

            chunk->header.payload_length = payload_length;
            chunk->header.message_type = *p++;

            if (fmt == RTMP_FMT_TYPE0) {
                pp = reinterpret_cast<char*>(&chunk->header.stream_id);
                pp[0] = *p++;
                pp[1] = *p++;
                pp[2] = *p++;
                pp[3] = *p++;
                tmss_info(
                        "header read completed. fmt={}, mh_size={}, ext_time={}, time={}, payload={}, type={}, sid={}",
                        fmt, mh_size, chunk->extended_timestamp,
                        chunk->header.timestamp, chunk->header.payload_length,
                        chunk->header.message_type, chunk->header.stream_id);
            } else {
                tmss_info(
                        "header read completed. fmt={}, mh_size={}, ext_time={}, time={}, payload={}, type={}",
                        fmt, mh_size, chunk->extended_timestamp,
                        chunk->header.timestamp, chunk->header.payload_length,
                        chunk->header.message_type);
            }
        } else {
            // fmt = 2, nothing left
            tmss_info(
                    "header read completed. fmt={}, mh_size={}, ext_time={}, time={}",
                    fmt, mh_size, chunk->extended_timestamp,
                    chunk->header.timestamp);
        }
    } else {
        // fmt = 3
        // update the timestamp even fmt=3 for first chunk packet
        if (is_first_chunk_of_msg && !chunk->extended_timestamp) {
            chunk->header.timestamp += chunk->header.timestamp_delta;
        }
        tmss_info("header read completed. fmt={}, size={}, ext_time={}", fmt,
                mh_size, chunk->extended_timestamp);
    }

    // read extended-timestamp
    if (chunk->extended_timestamp) {
        mh_size += 4;
        tmss_info("read header ext time. fmt={}, ext_time={}, mh_size={}",
                fmt, chunk->extended_timestamp, mh_size);
        // the ptr to the slice maybe invalid when grow()
        // reset the p to get 4bytes slice.
        char* p = pos;  //  in_buffer->read_slice(4);
        pos += 4;

        uint32_t timestamp = 0x00;
        char* pp = reinterpret_cast<char*>(&timestamp);
        pp[3] = *p++;
        pp[2] = *p++;
        pp[1] = *p++;
        pp[0] = *p++;

        tmss_info("read extended timestamp: {}", timestamp);

        // always use 31bits timestamp, for some server may use 32bits extended timestamp.
        // timestamp &= 0x7fffffff;
        if (!fix_rtmp_timestamp) {
            uint32_t chunk_timestamp = static_cast<uint32_t>(chunk->header.timestamp);
            /**
             * if chunk_timestamp<=0, the chunk previous packet has no extended-timestamp,
             * always use the extended timestamp.
             */
            /**
             * about the is_first_chunk_of_msg.
             * @remark, for the first chunk of message, always use the extended timestamp.
             */
            if (!is_first_chunk_of_msg && chunk_timestamp > 0
                && chunk_timestamp != timestamp) {
                mh_size -= 4;
                io_buffer->seek_read(-4);   //  in_buffer->skip(-4);
                tmss_info("no 4bytes extended timestamp in the continued chunk");
            } else {
                chunk->header.timestamp = timestamp;
            }
            tmss_info("header read ext_time completed. time={}",
                        chunk->header.timestamp);
        } else {
            uint32_t chunk_timestamp = static_cast<uint32_t>(chunk->extend_time);

            /**
             * if chunk_timestamp<=0, the chunk previous packet has no extended-timestamp,
             * always use the extended timestamp.
             * about the is_first_chunk_of_msg.
             * @remark, for the first chunk of message, always use the extended timestamp.
             */
            if (!is_first_chunk_of_msg && chunk_timestamp > 0
                    && chunk_timestamp != timestamp) {
                mh_size -= 4;
                io_buffer->seek_read(-4);   //  in_buffer->skip(-4);
                tmss_info("no 4bytes extended timestamp in the continued chunk chunk_timestamp:%u,"
                    " timestamp:{}", chunk_timestamp, timestamp);
            } else if (is_first_chunk_of_msg) {
                // renyu: chunk type=0的时候扩展时间戳就是绝对值，chunk type=1、2的时候为相对值
                if (fmt == RTMP_FMT_TYPE2 || fmt == RTMP_FMT_TYPE1) {
                    chunk->header.timestamp =
                         (static_cast<uint32_t>(chunk->header.timestamp) + timestamp);
                    chunk->header.timestamp_delta = timestamp;
                    tmss_info("fmt:{}, timestamp:%lld, delta={}",
                        fmt, chunk->header.timestamp, timestamp);

                } else if (fmt == RTMP_FMT_TYPE3) {
                    // to do: FIXME
                    // 当它跟在Type＝0的chunk后面时，表示和前一个chunk的时间戳都是相同的。什么时候连时间戳都相同呢
                    // 而当它跟在Type＝1或者Type＝2的chunk后面时，表示和前一个chunk的时间戳的差是相同的
                    chunk->header.timestamp = (uint32_t) ((uint32_t) chunk->header.timestamp + timestamp);
                    tmss_info("fmt:{}, timestamp:%lld, delta={}",
                        fmt, chunk->header.timestamp, timestamp);
                } else {
                    tmss_info("fmt:{}, pre_time:%lld, extend_timestamp:%u", fmt, chunk->header.timestamp, timestamp)
                    chunk->header.timestamp = timestamp;
                    chunk->header.timestamp_delta = 0;  // fmt = 0, dalta = 0 or next fmt = 3 will error
                }
            }
        }
        chunk->extend_time = timestamp;

        tmss_info("header read ext_time completed. time={}",
                chunk->header.timestamp);
        tmss_info("fix_rtmp_timestamp:{}, extend timestamp fmt:{}, timestamp:%lld, delta:{}, extend_timestamp:%u",
                fix_rtmp_timestamp ? 1 : 0,
                fmt, chunk->header.timestamp,
                chunk->header.timestamp_delta, timestamp);
    }

    tmss_info("final cid:{}, extend:{}, fmt:{}, timestamp:%lld, {}",
            chunk->cid, chunk->extended_timestamp ? 1 : 0,
            fmt, chunk->header.timestamp, chunk->header.timestamp_delta);
    // in a word, 31bits timestamp is ok.
    // convert extended timestamp to 31bits.
    // chunk->header.timestamp &= 0x7fffffff;
    chunk->header.timestamp &= 0xffffffff;

    // valid message, the payload_length is 24bits,
    // so it should never be negative.
    assert(chunk->header.payload_length >= 0);

    // copy header to msg
    chunk->msg->header = chunk->header;

    // increase the msg count, the chunk stream can accept fmt=1/2/3 message now.
    chunk->msg_count++;

    return ret;
}

int RtmpProtocolHandler::read_message_payload(std::shared_ptr<RtmpChunkStream> chunk,
        std::shared_ptr<RtmpMessage>& msg) {
    int ret = error_success;

    // empty message
    if (chunk->header.payload_length <= 0) {
        tmss_info(
                "get an empty RTMP " "message(type={}, size={}, time={}, sid={})",
                chunk->header.message_type, chunk->header.payload_length,
                chunk->header.timestamp, chunk->header.stream_id);

        msg = chunk->msg;
        chunk->msg.reset();

        return ret;
    }
    assert(chunk->header.payload_length > 0);

    // the chunk payload size.
    int payload_size = chunk->header.payload_length - chunk->msg->size;
    payload_size = Min(payload_size, in_chunk_size);
    tmss_info(
            "chunk payload size is {}, message_size={}, received_size={}, in_chunk_size={}",
            payload_size, chunk->header.payload_length, chunk->msg->size,
            in_chunk_size);

    // create msg payload if not initialized
    if (!chunk->msg->payload) {
        chunk->msg->create_payload(chunk->header.payload_length);   //  new buffer
    } else {
    }

    // read payload to buffer
    //  std::shared_ptr<char> payload(new char[payload_size]);
    //  char *pos = payload.get();
    //  if ((ret = in_buffer->grow(skt, payload_size)) != error_success) {
    if ((ret = io_buffer->read_bytes(chunk->msg->payload + chunk->msg->size, payload_size))
            != error_success) {
        tmss_info("read payload failed. required_size={}, ret={}",
                    payload_size, ret);
        if (ret != error_socket_timeout) {
                //  srs_is_client_gracefully_close(ret)) {
            tmss_error("read payload failed. required_size={}, ret={}",
                    payload_size, ret);
        }
        return ret;
    }
    //  memcpy(chunk->msg->payload + chunk->msg->size,
    //      pos, payload_size);    //  in_buffer->read_slice(payload_size)
    chunk->msg->size += payload_size;

    tmss_info("chunk payload read completed. payload_size={}", payload_size);

    // got entire RTMP message?
    if (chunk->header.payload_length == chunk->msg->size) {
        msg = chunk->msg;
        chunk->msg.reset();
        tmss_info(
                "get entire RTMP message(type={}, size={}, time={}, sid={})",
                chunk->header.message_type, chunk->header.payload_length,
                chunk->header.timestamp, chunk->header.stream_id);
        return ret;
    }

    tmss_info(
            "get partial RTMP message(type={}, size={}, time={}, sid={}), partial size={}",
            chunk->header.message_type, chunk->header.payload_length,
            chunk->header.timestamp, chunk->header.stream_id, chunk->msg->size);

    return ret;
}

int RtmpProtocolHandler::on_recv_message(std::shared_ptr<RtmpMessage>& msg,
        std::shared_ptr<RtmpPacket>& packet) {
    int ret = error_success;

    assert(msg != NULL);

    // try to response acknowledgement
    if ((ret = response_acknowledgement_message()) != error_success) {
        return ret;
    }

    switch (msg->header.message_type) {
        case RTMP_MSG_SetChunkSize:
        case RTMP_MSG_UserControlMessage:
        case RTMP_MSG_WindowAcknowledgementSize: {
                Buffer stream(msg->payload, msg->size);
                if ((ret = do_decode_message(msg->header, &stream, packet)) != error_success) {
                    tmss_error("decode packet from message payload failed. ret={}", ret);
                    return ret;
                }
                tmss_info("decode packet from message payload success.");
                break;
            }
        case RTMP_MSG_VideoMessage:
        case RTMP_MSG_AudioMessage:
        default:
            return ret;
    }

    assert(packet);

    switch (msg->header.message_type) {
    case RTMP_MSG_WindowAcknowledgementSize: {
        auto pkt =
                std::dynamic_pointer_cast<RtmpSetWindowAckSizePacket>(packet);
        assert(pkt != NULL);

        if (pkt->ackowledgement_window_size > 0) {
            in_ack_size.window = (uint32_t) pkt->ackowledgement_window_size;
            // @remark, we ignore this message, for user noneed to care.
            // but it's important for dev, for client/server will block if required
            // ack msg not arrived.
            tmss_info("set ack window size to {}",
                    pkt->ackowledgement_window_size);
        } else {
            tmss_warn("ignored. set ack window size is {}",
                    pkt->ackowledgement_window_size);
        }
        break;
    }
    case RTMP_MSG_SetChunkSize: {
        auto pkt =
                std::dynamic_pointer_cast<RtmpSetChunkSizePacket>(packet);
        assert(pkt != NULL);

        // for some server, the actual chunk size can greater than the max value(65536),
        // so we just warning the invalid chunk size, and actually use it is ok,
        // @see: https://github.com/ossrs/srs/issues/160
        if (pkt->chunk_size < TMSS_CONSTS_RTMP_MIN_CHUNK_SIZE
                || pkt->chunk_size > TMSS_CONSTS_RTMP_MAX_CHUNK_SIZE) {
            tmss_warn("accept chunk={}, should in [{}, {}], please see #160",
                    pkt->chunk_size, TMSS_CONSTS_RTMP_MIN_CHUNK_SIZE,
                    TMSS_CONSTS_RTMP_MAX_CHUNK_SIZE);
        }

        // @see: https://github.com/ossrs/srs/issues/541
        if (pkt->chunk_size < TMSS_CONSTS_RTMP_MIN_CHUNK_SIZE) {
            ret = error_rtmp_chunk_size;
            tmss_error("chunk size should be {}+, value={}. ret={}",
                    TMSS_CONSTS_RTMP_MIN_CHUNK_SIZE, pkt->chunk_size, ret);
            return ret;
        }

        in_chunk_size = pkt->chunk_size;
        tmss_info("in.chunk={}", pkt->chunk_size);

        break;
    }
    case RTMP_MSG_UserControlMessage: {
        auto pkt = std::dynamic_pointer_cast<RtmpUserControlPacket>(packet);
        assert(pkt != NULL);

        if (pkt->event_type == SrcPCUCSetBufferLength) {
            in_buffer_length = pkt->extra_data;
            tmss_info(
                    "buffer={}, in.ack={}, out.ack={}, in.chunk={}, out.chunk={}",
                    pkt->extra_data, in_ack_size.window, out_ack_size.window,
                    in_chunk_size, out_chunk_size);
        }
        if (pkt->event_type == SrcPCUCPingRequest) {
            if ((ret = response_ping_message(pkt->event_data)) != error_success) {
                return ret;
            }
        }
        break;
    }
    default:
        break;
    }

    return ret;
}

int RtmpProtocolHandler::do_decode_message(MessageHeader& header, Buffer* stream,
        std::shared_ptr<RtmpPacket>& packet) {
    int ret = error_success;
    // decode specified packet type
    if (header.is_amf0_command() || header.is_amf3_command()
            || header.is_amf0_data() || header.is_amf3_data()) {
        tmss_info("start to decode AMF0/AMF3 command message.");

        // skip 1bytes to decode the amf3 command.
        if (header.is_amf3_command() && stream->read_require(1)) {
            tmss_info("skip 1bytes to decode AMF3 command");
            stream->seek_read(1);
        }

        // amf0 command message.
        // need to read the command name.
        std::string command;
        if ((ret = amf0_read_string(stream, command)) != error_success) {
            tmss_error("decode AMF0/AMF3 command name failed. ret={}", ret);
            return ret;
        }
        tmss_info("AMF0/AMF3 command message, command_name={}",
                command.c_str());

        // result/error packet
        if (command == RTMP_AMF0_COMMAND_RESULT
                || command == RTMP_AMF0_COMMAND_ERROR) {
            double transactionId = 0.0;
            if ((ret = amf0_read_number(stream, transactionId))
                    != error_success) {
                tmss_error("decode AMF0/AMF3 transcationId failed. ret={}", ret);
                return ret;
            }
            tmss_info("AMF0/AMF3 command id, transcationId=%.2f",
                    transactionId);

            // reset stream, for header read completed.
            stream->reset();    //  stream->seek_read(-1 * stream->rcurrent());
            if (header.is_amf3_command()) {
                stream->seek_read(1);
            }

            // find the call name
            if (requests.find(transactionId) == requests.end()) {
                ret = error_rtmp_no_request;
                tmss_error("decode AMF0/AMF3 request failed. ret={}", ret);
                return ret;
            }

            std::string request_name = requests[transactionId];
            tmss_info("AMF0/AMF3 request parsed. request_name={}",
                    request_name.c_str());

            if (request_name == RTMP_AMF0_COMMAND_CONNECT) {
                tmss_info("decode the AMF0/AMF3 response command({} message).",
                        request_name.c_str());
                packet = std::make_shared<RtmpConnectAppResPacket>();
                return packet->decode(stream);
            } else if (request_name == RTMP_AMF0_COMMAND_CREATE_STREAM) {
                tmss_info("decode the AMF0/AMF3 response command({} message).",
                        request_name.c_str());
                packet = std::make_shared<RtmpCreateStreamResPacket>(0, 0);
                return packet->decode(stream);
            } else if (request_name == RTMP_AMF0_COMMAND_RELEASE_STREAM
                    || request_name == RTMP_AMF0_COMMAND_FC_PUBLISH
                    || request_name == RTMP_AMF0_COMMAND_UNPUBLISH) {
                tmss_info("decode the AMF0/AMF3 response command({} message).",
                        request_name.c_str());
                packet = std::make_shared<RtmpFMLEStartResPacket>(0);
                return packet->decode(stream);
            } else {
                ret = error_rtmp_no_request;
                tmss_error(
                        "decode AMF0/AMF3 request failed.request_name={}, transactionId={}, ret={}",
                        request_name.c_str(), transactionId, ret);
                return ret;
            }
        }

        // reset to zero(amf3 to 1) to restart decode.
        stream->reset();    //  stream->seek_read(-1 * stream->rcurrent());
        if (header.is_amf3_command()) {
            stream->seek_read(1);
        }

        if ((ret = do_decode_command(header, command, stream, packet)) != error_success) {
            tmss_error("decode command error, ret={}", ret);
            return ret;
        }

        // default packet to drop message.
        tmss_info("drop the AMF0/AMF3 command message, command_name={}",
                command.c_str());
        packet = std::make_shared<RtmpPacket>();
        return ret;
    } else if (header.is_user_control_message()) {
        tmss_info("start to decode user control message.");
        packet = std::make_shared<RtmpUserControlPacket>();
        return packet->decode(stream);
    } else if (header.is_window_ackledgement_size()) {
        tmss_info("start to decode set ack window size message.");
        packet = std::make_shared<RtmpSetWindowAckSizePacket>();
        return packet->decode(stream);
    } else if (header.is_set_chunk_size()) {
        tmss_info("start to decode set chunk size message.");
        packet = std::make_shared<RtmpSetChunkSizePacket>();
        return packet->decode(stream);
    } else {
        if (!header.is_set_peer_bandwidth() && !header.is_ackledgement()) {
            tmss_info("drop unknown message, type={}", header.message_type);
        }
    }

    return ret;
}

int RtmpProtocolHandler::do_decode_command(MessageHeader& header,
        const std::string& command, Buffer* stream,
        std::shared_ptr<RtmpPacket>& packet) {
    int ret = error_success;

    // decode command object.
    if (command == RTMP_AMF0_COMMAND_CONNECT) {
        tmss_info("decode the AMF0/AMF3 command(connect vhost/app message).");
        packet = std::make_shared<RtmpConnectAppPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_CREATE_STREAM) {
        tmss_info("decode the AMF0/AMF3 command(createBuffer message).");
        packet = std::make_shared<RtmpCreateStreamPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_PLAY) {
        tmss_info("decode the AMF0/AMF3 command(paly message).");
        packet = std::make_shared<RtmpPlayPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_PAUSE) {
        tmss_info("decode the AMF0/AMF3 command(pause message).");
        packet = std::make_shared<RtmpPausePacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_RELEASE_STREAM) {
        tmss_info("decode the AMF0/AMF3 command(FMLE releaseBuffer message).");
        packet = std::make_shared<RtmpFMLEStartPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_FC_PUBLISH) {
        tmss_info("decode the AMF0/AMF3 command(FMLE FCPublish message).");
        packet = std::make_shared<RtmpFMLEStartPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_PUBLISH) {
        tmss_info("decode the AMF0/AMF3 command(publish message).");
        packet = std::make_shared<RtmpPublishPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_UNPUBLISH) {
        tmss_info("decode the AMF0/AMF3 command(unpublish message).");
        packet = std::make_shared<RtmpFMLEStartPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_ON_STATUS) {
        tmss_info("decode the AMF0/AMF3 command(onStatus message).");
        packet = std::make_shared<RtmpOnStatusCallPacket>();
        if (packet->decode(stream) != error_success) {
            packet = std::make_shared<RtmpPacket>();
        }
        return ret;
    } else if (command == TMSS_CONSTS_RTMP_SET_DATAFRAME
            || command == TMSS_CONSTS_RTMP_ON_METADATA) {
        tmss_info("decode the AMF0/AMF3 data(onMetaData message).");
        packet = std::make_shared<RtmpOnMetaDataPacket>();
        return packet->decode(stream);
    } else if (command == TMSS_BW_CHECK_FINISHED
            || command == TMSS_BW_CHECK_PLAYING
            || command == TMSS_BW_CHECK_PUBLISHING
            || command == TMSS_BW_CHECK_STARTING_PLAY
            || command == TMSS_BW_CHECK_STARTING_PUBLISH
            || command == TMSS_BW_CHECK_START_PLAY
            || command == TMSS_BW_CHECK_START_PUBLISH
            || command == TMSS_BW_CHECK_STOPPED_PLAY
            || command == TMSS_BW_CHECK_STOP_PLAY
            || command == TMSS_BW_CHECK_STOP_PUBLISH
            || command == TMSS_BW_CHECK_STOPPED_PUBLISH
            || command == TMSS_BW_CHECK_FINAL) {
        tmss_info("decode the AMF0/AMF3 band width check message.");
        packet = std::make_shared<RtmpBandwidthPacket>();
        return packet->decode(stream);
    } else if (command == RTMP_AMF0_COMMAND_CLOSE_STREAM) {
        tmss_info("decode the AMF0/AMF3 closeBuffer message.");
        packet = std::make_shared<RtmpCloseStreamPacket>();
        return packet->decode(stream);
    } else if (header.is_amf0_command() || header.is_amf3_command()) {
        tmss_info("decode the AMF0/AMF3 call message.");
        packet = std::make_shared<RtmpCallPacket>();
        return packet->decode(stream);
    } else if (header.is_amf0_data() || header.is_amf3_data()) {
        // renyu: 所有未知非标准协议的AMF数据，都取一下name，根据配置判断是否透传
        //        注意客户自定义AMF数据可能解析出错，直接忽略处理不要返回错误码造成断流
        tmss_info("received the AMF0/AMF3 custom message, decode the name.");
        packet = std::make_shared<RtmpOnCustomAmfDataPacket>();
        if (packet->decode(stream) != error_success) {
            packet = std::make_shared<RtmpPacket>();
        }
        return ret;
    }
    return ret;
}

int RtmpProtocolHandler::response_acknowledgement_message() {
    int ret = error_success;

    if (in_ack_size.window <= 0) {
        return ret;
    }

    // ignore when delta bytes not exceed half of window(ack size).
    uint32_t delta = static_cast<uint32_t>(conn->get_recv_bytes()
            - in_ack_size.nb_recv_bytes);
    if (delta < in_ack_size.window / 2) {
        return ret;
    }
    in_ack_size.nb_recv_bytes = conn->get_recv_bytes();

    // when the sequence number overflow, reset it.
    uint32_t sequence_number = in_ack_size.sequence_number + delta;
    if (sequence_number > 0xf0000000) {
        sequence_number = delta;
    }
    in_ack_size.sequence_number = sequence_number;

    std::shared_ptr<RtmpAcknowledgementPacket> pkt =
        std::make_shared<RtmpAcknowledgementPacket>();
    pkt->sequence_number = sequence_number;

    // cache the message and use flush to send.
    if (!auto_response_when_recv) {
        manual_response_queue.push_back(pkt);
        return ret;
    }

    // use underlayer api to send, donot flush again.
    if ((ret = send_packet(pkt, 0)) != error_success) {
        tmss_error("send acknowledgement failed. ret={}", ret);
        return ret;
    }
    tmss_info("send acknowledgement success.");

    return ret;
}

int RtmpProtocolHandler::response_ping_message(int32_t timestamp) {
    int ret = error_success;

    tmss_info("get a ping request, response it. timestamp={}", timestamp);

    std::shared_ptr<RtmpUserControlPacket> pkt =
        std::make_shared<RtmpUserControlPacket>();

    pkt->event_type = SrcPCUCPingResponse;
    pkt->event_data = timestamp;

    // cache the message and use flush to send.
    if (!auto_response_when_recv) {
        manual_response_queue.push_back(pkt);
        return ret;
    }

    // use underlayer api to send, donot flush again.
    if ((ret = send_packet(pkt, 0)) != error_success) {
        tmss_error("send ping response failed. ret={}", ret);
        return ret;
    }
    tmss_info("send ping response success.");

    return ret;
}

int chunk_header_c0(int perfer_cid, uint32_t timestamp, int32_t payload_length,
    int8_t message_type, int32_t stream_id,
    char* cache, int nb_cache) {
    // to directly set the field.
    char* pp = NULL;

    // generate the header.
    char* p = cache;

    // no header.
    if (nb_cache < TMSS_CONSTS_RTMP_MAX_FMT0_HEADER_SIZE) {
        return 0;
    }

    // write new chunk stream header, fmt is 0
    *p++ = 0x00 | (perfer_cid & 0x3F);

    // chunk message header, 11 bytes
    // timestamp, 3bytes, big-endian
    if (timestamp < RTMP_EXTENDED_TIMESTAMP) {
        pp = reinterpret_cast<char*>(&timestamp);
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    } else {
        *p++ = 0xFF;
        *p++ = 0xFF;
        *p++ = 0xFF;
    }

    // message_length, 3bytes, big-endian
    pp = reinterpret_cast<char*>(&payload_length);
    *p++ = pp[2];
    *p++ = pp[1];
    *p++ = pp[0];

    // message_type, 1bytes
    *p++ = message_type;

    // stream_id, 4bytes, little-endian
    pp = reinterpret_cast<char*>(&stream_id);
    *p++ = pp[0];
    *p++ = pp[1];
    *p++ = pp[2];
    *p++ = pp[3];

    // for c0
    // chunk extended timestamp header, 0 or 4 bytes, big-endian
    //
    // for c3:
    // chunk extended timestamp header, 0 or 4 bytes, big-endian
    // 6.1.3. Extended Timestamp
    // This field is transmitted only when the normal time stamp in the
    // chunk message header is set to 0x00ffffff. If normal time stamp is
    // set to any value less than 0x00ffffff, this field MUST NOT be
    // present. This field MUST NOT be present if the timestamp field is not
    // present. Type 3 chunks MUST NOT have this field.
    // adobe changed for Type3 chunk:
    //        FMLE always sendout the extended-timestamp,
    //        must send the extended-timestamp to FMS,
    //        must send the extended-timestamp to flash-player.
    // @see: ngx_rtmp_prepare_message
    // @see: http://blog.csdn.net/win_lin/article/details/13363699
    // to do: FIXME: extract to outer.
    if (timestamp >= RTMP_EXTENDED_TIMESTAMP) {
        pp = reinterpret_cast<char*>(&timestamp);
        *p++ = pp[3];
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    }

    // always has header
    return p - cache;
}

int chunk_header_c3(int perfer_cid, uint32_t timestamp,
    char* cache, int nb_cache) {
    // to directly set the field.
    char* pp = NULL;

    // generate the header.
    char* p = cache;

    // no header.
    if (nb_cache < TMSS_CONSTS_RTMP_MAX_FMT3_HEADER_SIZE) {
        return 0;
    }

    // write no message header chunk stream, fmt is 3
    // @remark, if perfer_cid > 0x3F, that is, use 2B/3B chunk header,
    // TMSS will rollback to 1B chunk header.
    *p++ = 0xC0 | (perfer_cid & 0x3F);

    // for c0
    // chunk extended timestamp header, 0 or 4 bytes, big-endian
    //
    // for c3:
    // chunk extended timestamp header, 0 or 4 bytes, big-endian
    // 6.1.3. Extended Timestamp
    // This field is transmitted only when the normal time stamp in the
    // chunk message header is set to 0x00ffffff. If normal time stamp is
    // set to any value less than 0x00ffffff, this field MUST NOT be
    // present. This field MUST NOT be present if the timestamp field is not
    // present. Type 3 chunks MUST NOT have this field.
    // adobe changed for Type3 chunk:
    //        FMLE always sendout the extended-timestamp,
    //        must send the extended-timestamp to FMS,
    //        must send the extended-timestamp to flash-player.
    // @see: ngx_rtmp_prepare_message
    // @see: http://blog.csdn.net/win_lin/article/details/13363699
    // to do: FIXME: extract to outer.
    if (timestamp >= RTMP_EXTENDED_TIMESTAMP) {
        pp = reinterpret_cast<char*>(&timestamp);
        *p++ = pp[3];
        *p++ = pp[2];
        *p++ = pp[1];
        *p++ = pp[0];
    }

    // always has header
    return p - cache;
}

}   // namespace tmss

