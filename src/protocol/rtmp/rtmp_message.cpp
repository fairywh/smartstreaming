/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include <protocol/rtmp/rtmp_message.hpp>
#include <utility>
#include <defs/err.hpp>
#include <log/log.hpp>
#include <util/util.hpp>
#include <defs/tmss_def.hpp>
#include <rtmp_def.hpp>

namespace tmss {
MessageHeader::MessageHeader() {
    message_type = 0;
    payload_length = 0;
    timestamp_delta = 0;
    stream_id = 0;

    timestamp = 0;
    seq = 0;
    code = 0;
    duration = 0;
    // we always use the connection chunk-id
    perfer_cid = RTMP_CID_OverConnection;
    tencent_metadata = false;
    is_custom_amfdata = false;
}

MessageHeader::~MessageHeader() {
}

bool MessageHeader::is_audio() {
    return message_type == RTMP_MSG_AudioMessage;
}

bool MessageHeader::is_video() {
    return message_type == RTMP_MSG_VideoMessage;
}

bool MessageHeader::is_amf0_command() {
    return message_type == RTMP_MSG_AMF0CommandMessage;
}

bool MessageHeader::is_amf0_data() {
    return message_type == RTMP_MSG_AMF0DataMessage;
}

bool MessageHeader::is_amf3_command() {
    return message_type == RTMP_MSG_AMF3CommandMessage;
}

bool MessageHeader::is_amf3_data() {
    return message_type == RTMP_MSG_AMF3DataMessage;
}

bool MessageHeader::is_window_ackledgement_size() {
    return message_type == RTMP_MSG_WindowAcknowledgementSize;
}

bool MessageHeader::is_ackledgement() {
    return message_type == RTMP_MSG_Acknowledgement;
}

bool MessageHeader::is_set_chunk_size() {
    return message_type == RTMP_MSG_SetChunkSize;
}

bool MessageHeader::is_user_control_message() {
    return message_type == RTMP_MSG_UserControlMessage;
}

bool MessageHeader::is_set_peer_bandwidth() {
    return message_type == RTMP_MSG_SetPeerBandwidth;
}

bool MessageHeader::is_aggregate() {
    return message_type == RTMP_MSG_AggregateMessage;
}

bool MessageHeader::is_flv_header() {
    return message_type == FLV_MSG_HeaderMessage;
}

void MessageHeader::initialize_amf0_script(int size, int stream) {
    message_type = RTMP_MSG_AMF0DataMessage;
    payload_length = (int32_t)size;
    timestamp_delta = (int32_t)0;
    timestamp = (int64_t)0;
    stream_id = (int32_t)stream;

    // amf0 script use connection2 chunk-id
    perfer_cid = RTMP_CID_OverConnection2;
}

void MessageHeader::initialize_audio(int size, uint32_t time, int stream) {
    message_type = RTMP_MSG_AudioMessage;
    payload_length = (int32_t)size;
    timestamp_delta = (int32_t)time;
    timestamp = (int64_t)time;
    stream_id = (int32_t)stream;

    // audio chunk-id
    perfer_cid = RTMP_CID_Audio;
}

void MessageHeader::initialize_video(int size, uint32_t time, int stream) {
    message_type = RTMP_MSG_VideoMessage;
    payload_length = (int32_t)size;
    timestamp_delta = (int32_t)time;
    timestamp = (int64_t)time;
    stream_id = (int32_t)stream;

    // video chunk-id
    perfer_cid = RTMP_CID_Video;
}
RtmpMessage::RtmpMessage() {
    payload = NULL;
    size = 0;
}

RtmpMessage::~RtmpMessage() {
    freepa(payload);
}


void RtmpMessage::create_payload(int size) {
    freepa(payload);

    payload = new char[size];
    tmss_info("create payload for RTMP message. size=%d", size);
}

int RtmpMessage::create(char type, int64_t timestamp, char * payload, int size) {
    int ret = error_success;

    int8_t tag_type;
    int perfer_cid;
    if (type == CodecFlvTagAudio) {
        tag_type = RTMP_MSG_AudioMessage;
        perfer_cid = RTMP_CID_Audio;
    } else if (type == CodecFlvTagVideo) {
        tag_type = RTMP_MSG_VideoMessage;
        perfer_cid = RTMP_CID_Video;
    } else if (type == CodecFlvTagScript) {
        tag_type = RTMP_MSG_AMF0DataMessage;
        perfer_cid = RTMP_CID_OverConnection2;
    } else {
        ret = error_tag_type_invalid;
        tmss_error("rtmp unknown tag type={} ret={}", type, ret);
        return ret;
    }

    header.message_type = tag_type;
    header.payload_length = size;
    //  header.proto_type = proto_type;
    header.timestamp = timestamp;
    header.perfer_cid = perfer_cid;
    this->size = size;
    this->payload = payload;

    return ret;
}

void RtmpMessage::set_payload(char* buf) {
    freepa(payload);

    payload = buf;
}

RtmpChunkStream::RtmpChunkStream(int _cid) {
    fmt = 0;
    cid = _cid;
    extended_timestamp = false;
    msg_count = 0;
    extend_time = 0;
}

RtmpChunkStream::~RtmpChunkStream() {
}

}   // namespace tmss