/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/6
 *        Author:  rainwu
 *
 * =====================================================================================
 */
#pragma once

#include <string>
#include <map>
#include <memory>
#include "util/util.hpp"
#include <defs/err.hpp>

namespace tmss {
class MessageHeader {
 public:
    //  int8_t proto_type;
    uint64_t seq;
    int code;
    double duration;

    /**
    * 3bytes.
    * Three-byte field that contains a timestamp delta of the message.
    * @remark, only used for decoding message from chunk stream.
    */
    int32_t timestamp_delta;
    /**
    * 3bytes.
    * Three-byte field that represents the size of the payload in bytes.
    * It is set in big-endian format.
    */
    int32_t payload_length;
    /**
    * 1byte.
    * One byte field to represent the message type. A range of type IDs
    * (1-7) are reserved for protocol control messages.
    */
    int8_t message_type;
    /**
    * 4bytes.
    * Four-byte field that identifies the stream of the message. These
    * bytes are set in little-endian format.
    */
    int32_t stream_id;

    /**
    * Four-byte field that contains a timestamp of the message.
    * The 4 bytes are packed in the big-endian order.
    * @remark, used as calc timestamp when decode and encode time.
    * @remark, we use 64bits for large time for jitter detect and hls.
    */
    int64_t timestamp;

 public:
    /**
    * get the perfered cid(chunk stream id) which sendout over.
    * set at decoding, and canbe used for directly send message,
    * for example, dispatch to all connections.
    */
    int perfer_cid;

    bool tencent_metadata;
    bool is_custom_amfdata;

 public:
    MessageHeader();
    virtual ~MessageHeader();

 public:
    bool is_audio();
    bool is_video();
    bool is_amf0_command();
    bool is_amf0_data();
    bool is_amf3_command();
    bool is_amf3_data();
    bool is_window_ackledgement_size();
    bool is_ackledgement();
    bool is_set_chunk_size();
    bool is_user_control_message();
    bool is_set_peer_bandwidth();
    bool is_aggregate();
    bool is_flv_header();

 public:
    /**
    * create a amf0 script header, set the size and stream_id.
    */
    void initialize_amf0_script(int size, int stream);
    /**
    * create a audio header, set the size, timestamp and stream_id.
    */
    void initialize_audio(int size, uint32_t time, int stream);
    /**
    * create a video header, set the size, timestamp and stream_id.
    */
    void initialize_video(int size, uint32_t time, int stream);
};

/**
 * message is raw data RTMP message, bytes oriented,
 * protcol always recv RTMP message, and can send RTMP message or RTMP packet.
 * the common message is read from underlay protocol sdk.
 * while the shared ptr message used to copy and send.
 */
class RtmpMessage {
    // 4.1. Message Header
 public:
    MessageHeader header;
    // 4.2. Message Payload

 public:
    /**
     * current message parsed size,
     *       size <= header.payload_length
     * for the payload maybe sent in multiple chunks.
     */
    int size;
    /**
     * the payload of message, the RtmpMessage never know about the detail of payload,
     * user must use Protocol.decode_message to get concrete packet.
     * @remark, not all message payload can be decoded to packet. for example,
     *       video/audio packet use raw bytes, no video/audio packet.
     */
    char* payload;

 public:
    RtmpMessage();
    virtual ~RtmpMessage();

 public:
    /**
     * alloc the payload to specified size of bytes.
     */
    virtual void create_payload(int size);

    //  create shared ptr message
    virtual int create(char type,
        int64_t timestamp, char* payload, int size);
        //  int8_t proto_type = SRS_MSG_PROTO_FLV);

    //  virtual int reset_paload(const char *input, int size);

    virtual void set_payload(char* buf);
};

/**
 * incoming chunk stream maybe interlaced,
 * use the chunk stream to cache the input RTMP chunk streams.
 */
class RtmpChunkStream {
 public:
    /**
     * represents the basic header fmt,
     * which used to identify the variant message header type.
     */
    char fmt;
    /**
     * represents the basic header cid,
     * which is the chunk stream id.
     */
    int cid;
    /**
     * cached message header
     */
    MessageHeader header;
    /**
     * whether the chunk message header has extended timestamp.
     */
    bool extended_timestamp;
    /**
     * partially read message.
     */
    std::shared_ptr<RtmpMessage> msg;
    /**
     * decoded msg count, to identify whether the chunk stream is fresh.
     */
    int64_t msg_count;

    // 修复rtmp的解析时间戳的问题
    uint32_t extend_time;

 public:
    explicit RtmpChunkStream(int _cid);
    virtual ~RtmpChunkStream();
};

}  // namespace tmss

