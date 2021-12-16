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
#include "util/util.hpp"
#include <defs/err.hpp>
#include <defs/tmss_def.hpp>
#include <rtmp_def.hpp>
#include <rtmp/rtmp_amf.hpp>
#include <rtmp/rtmp_message.hpp>
#include <log/log.hpp>
#include <net/tmss_conn.hpp>

/**
 * amf0 command message, command name macros
 */
#define RTMP_AMF0_COMMAND_CONNECT               "connect"
#define RTMP_AMF0_COMMAND_CREATE_STREAM         "createStream"
#define RTMP_AMF0_COMMAND_CLOSE_STREAM          "closeStream"
#define RTMP_AMF0_COMMAND_PLAY                  "play"
#define RTMP_AMF0_COMMAND_PAUSE                 "pause"
#define RTMP_AMF0_COMMAND_ON_BW_DONE            "onBWDone"
#define RTMP_AMF0_COMMAND_ON_STATUS             "onStatus"
#define RTMP_AMF0_COMMAND_RESULT                "_result"
#define RTMP_AMF0_COMMAND_ERROR                 "_error"
#define RTMP_AMF0_COMMAND_RELEASE_STREAM        "releaseStream"
#define RTMP_AMF0_COMMAND_FC_PUBLISH            "FCPublish"
#define RTMP_AMF0_COMMAND_UNPUBLISH             "FCUnpublish"
#define RTMP_AMF0_COMMAND_PUBLISH               "publish"
#define RTMP_AMF0_DATA_SAMPLE_ACCESS            "|RtmpSampleAccess"

/**
 * onStatus consts.
 */
#define StatusLevel                             "level"
#define StatusCode                              "code"
#define StatusDescription                       "description"
#define StatusDetails                           "details"
#define StatusClientId                          "clientid"
// status value
#define StatusLevelStatus                       "status"
// status error
#define StatusLevelError                        "error"
// code value
#define StatusCodeConnectSuccess                "NetConnection.Connect.Success"
#define StatusCodeConnectRejected               "NetConnection.Connect.Rejected"
#define StatusCodeStreamReset                   "NetStream.Play.Reset"
#define StatusCodeStreamStart                   "NetStream.Play.Start"
#define StatusCodeStreamPause                   "NetStream.Pause.Notify"
#define StatusCodeStreamUnpause                 "NetStream.Unpause.Notify"
#define StatusCodePublishStart                  "NetStream.Publish.Start"
#define StatusCodeDataStart                     "NetStream.Data.Start"
#define StatusCodeUnpublishSuccess              "NetStream.Unpublish.Success"


// server play control
#define TMSS_BW_CHECK_START_PLAY                 "onSrsBandCheckStartPlayBytes"
#define TMSS_BW_CHECK_STARTING_PLAY              "onSrsBandCheckStartingPlayBytes"
#define TMSS_BW_CHECK_STOP_PLAY                  "onSrsBandCheckStopPlayBytes"
#define TMSS_BW_CHECK_STOPPED_PLAY               "onSrsBandCheckStoppedPlayBytes"

// server publish control
#define TMSS_BW_CHECK_START_PUBLISH              "onSrsBandCheckStartPublishBytes"
#define TMSS_BW_CHECK_STARTING_PUBLISH           "onSrsBandCheckStartingPublishBytes"
#define TMSS_BW_CHECK_STOP_PUBLISH               "onSrsBandCheckStopPublishBytes"
// @remark, flash never send out this packet, for its queue is full.
#define TMSS_BW_CHECK_STOPPED_PUBLISH            "onSrsBandCheckStoppedPublishBytes"

// EOF control.
// the report packet when check finished.
#define TMSS_BW_CHECK_FINISHED                   "onSrsBandCheckFinished"
// @remark, flash never send out this packet, for its queue is full.
#define TMSS_BW_CHECK_FINAL                      "finalClientPacket"

// data packets
#define TMSS_BW_CHECK_PLAYING                    "onSrsBandCheckPlaying"
#define TMSS_BW_CHECK_PUBLISHING                 "onSrsBandCheckPublishing"

#define TMSS_CONSTS_RTMP_ON_METADATA              "onMetaData"
#define TMSS_CONSTS_RTMP_SET_DATAFRAME            "@setDataFrame"

namespace tmss {
class RtmpHandshakeBytes {
 public:
    // [1+1536]
    char* c0c1;
    // [1+1536+1536]
    char* s0s1s2;
    // [1536]
    char* c2;
 public:
    RtmpHandshakeBytes();
    virtual ~RtmpHandshakeBytes();
 public:
    virtual int read_c0c1(std::shared_ptr<IClientConn> conn);
    virtual int read_s0s1s2(std::shared_ptr<IClientConn> conn);
    virtual int read_c2(std::shared_ptr<IClientConn> conn);
    virtual int create_c0c1();
    virtual int create_s0s1s2(const char* c1 = NULL);
    virtual int create_c2();
};

class RtmpPacket {
 public:
     RtmpPacket();
     virtual ~RtmpPacket() = default;

     virtual int encode(int& size, char*& payload);
     virtual int decode(Buffer* stream);

 public:
    /**
     * the cid(chunk id) specifies the chunk to send data over.
     * generally, each message perfer some cid, for example,
     * all protocol control messages perfer RTMP_CID_ProtocolControl,
     * SetWindowAckSizePacket is protocol control message.
     */
    virtual int get_prefer_cid();
    /**
     * subpacket must override to provide the right message type.
     * the message type set the RTMP message type in header.
     */
    virtual int get_message_type();

 protected:
    /**
     * subpacket can override to calc the packet size.
     */
    virtual int get_size();

    /**
     * subpacket can override to encode the payload to stream.
     * @remark never invoke the super.encode_packet, it always failed.
     */
    virtual int encode_packet(Buffer* stream);
};

class RtmpConnectAppPacket : public RtmpPacket {
 public:
    RtmpConnectAppPacket();
    ~RtmpConnectAppPacket();
    virtual int decode(Buffer* stream);

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);

 public:
    std::string command_name;

    double transaction_id;

    std::shared_ptr<Amf0Object> command_object;

    std::shared_ptr<Amf0Object> args;
};

class RtmpConnectAppResPacket : public RtmpPacket {
 public:
    RtmpConnectAppResPacket();
    ~RtmpConnectAppResPacket();
    virtual int decode(Buffer* stream);

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);

 public:
    /**
     * _result or _error; indicates whether the response is result or error.
     */
    std::string command_name;
    /**
     * Transaction ID is 1 for call connect responses
     */
    double transaction_id;
    /**
     * Name-value pairs that describe the properties(fmsver etc.) of the connection.
     * @remark, never be NULL.
     */
    std::shared_ptr<Amf0Object> props;
    /**
     * Name-value pairs that describe the response from|the server. 'code',
     * 'level', 'description' are names of few among such information.
     * @remark, never be NULL.
     */
    std::shared_ptr<Amf0Object> info;
};

class RtmpSetWindowAckSizePacket : public RtmpPacket {
 public:
    RtmpSetWindowAckSizePacket();
    ~RtmpSetWindowAckSizePacket();
    virtual int decode(Buffer* stream);

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);

 public:
    int32_t ackowledgement_window_size;
};

class RtmpPlayPacket : public RtmpPacket {
 public:
    RtmpPlayPacket();
    ~RtmpPlayPacket();
    virtual int decode(Buffer* stream);

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
     virtual int encode_packet(Buffer* stream);

 public:
     /**
     * Name of the command. Set to "play".
     */
     std::string command_name;

     /**
     * Transaction ID set to 0.
     */
     double transaction_id;

     /**
     * Command information does not exist. Set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
     std::shared_ptr<Amf0Any> command_object;  // null

     std::string stream_name;

     /**
     * An optional parameter that specifies the start time in seconds.
     * The default value is -2, which means the subscriber first tries to play the live
     *       stream specified in the Stream Name field. If a live stream of that name is
     *       not found, it plays the recorded stream specified in the Stream Name field.
     * If you pass -1 in the Start field, only the live stream specified in the Stream
     *       Name field is played.
     * If you pass 0 or a positive number in the Start field, a recorded stream specified
     *       in the Stream Name field is played beginning from the time specified in the
     *       Start field.
     * If no recorded stream is found, the next item in the playlist is played.
     */
    double start;
    /**
     * An optional parameter that specifies the duration of playback in seconds.
     * The default value is -1. The -1 value means a live stream is played until it is no
     *       longer available or a recorded stream is played until it ends.
     * If u pass 0, it plays the single frame since the time specified in the Start field
     *       from the beginning of a recorded stream. It is assumed that the value specified
     *       in the Start field is equal to or greater than 0.
     * If you pass a positive number, it plays a live stream for the time period specified
     *       in the Duration field. After that it becomes available or plays a recorded
     *       stream for the time specified in the Duration field. (If a stream ends before the
     *       time specified in the Duration field, playback ends when the stream ends.)
     * If you pass a negative number other than -1 in the Duration field, it interprets the
     *       value as if it were -1.
     */
    double duration;
    /**
     * An optional Boolean value or number that specifies whether to flush any
     * previous playlist.
     */
    bool reset;
};

/**
 * response for PlayPacket.
 * @remark, user must set the stream_id in header.
 */
class RtmpPlayResPacket: public RtmpPacket {
 public:
    /**
     * Name of the command. If the play command is successful, the command
     * name is set to onStatus.
     */
    std::string command_name;
    /**
     * Transaction ID set to 0.
     */
    double transaction_id;
    /**
     * Command information does not exist. Set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null
    /**
     * If the play command is successful, the client receives OnStatus message from
     * server which is NetStream.Play.Start. If the specified stream is not found,
     * NetStream.Play.StreamNotFound is received.
     * @remark, never be NULL, an AMF0 object instance.
     */
    std::shared_ptr<Amf0Object> desc;

 public:
    RtmpPlayResPacket();
    virtual ~RtmpPlayResPacket();
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

class RtmpUserControlPacket : public RtmpPacket {
 public:
    RtmpUserControlPacket();
    ~RtmpUserControlPacket();
    virtual int decode(Buffer* stream);

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);

 public:
     /**
      * Event type is followed by Event data.
      */
     int16_t event_type;
     /**
      * the event data generally in 4bytes.
      */
     int32_t event_data;
     /**
      * 4bytes if event_type is SetBufferLength; otherwise 0.
      */
     int32_t extra_data;
};

class RtmpSetChunkSizePacket : public RtmpPacket {
 public:
    RtmpSetChunkSizePacket();
    ~RtmpSetChunkSizePacket();
    virtual int decode(Buffer* stream);

 public:
    // encode functions for concrete packet to override.
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);

 public:
    int32_t chunk_size;
};

// 5.6. Set Peer Bandwidth (6)
enum RtmpPeerBandwidthType {
    // The sender can mark this message hard (0), soft (1), or dynamic (2)
    // using the Limit type field.
    RtmpPeerBandwidthHard = 0,
    RtmpPeerBandwidthSoft = 1,
    RtmpPeerBandwidthDynamic = 2,
};

class RtmpSetPeerBandwidthPacket : public RtmpPacket {
 public:
    RtmpSetPeerBandwidthPacket();
    ~RtmpSetPeerBandwidthPacket();

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);

 public:
    int32_t bandwidth;
    // @see: PeerBandwidthType
    int8_t type;
};

class RtmpCreateStreamPacket: public RtmpPacket {
 public:
    /**
     * Name of the command. Set to "createStream".
     */
    std::string command_name;
    /**
     * Transaction ID of the command.
     */
    double transaction_id;
    /**
     * If there exists any command info this is set, else this is set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null

 public:
    RtmpCreateStreamPacket();
    virtual ~RtmpCreateStreamPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

class RtmpCreateStreamResPacket: public RtmpPacket {
 public:
    /**
     * _result or _error; indicates whether the response is result or error.
     */
    std::string command_name;
    /**
     * ID of the command that response belongs to.
     */
    double transaction_id;
    /**
     * If there exists any command info this is set, else this is set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null
    /**
     * The return value is either a stream ID or an error information object.
     */
    double stream_id;

 public:
    RtmpCreateStreamResPacket(double _transaction_id, double _stream_id);
    virtual ~RtmpCreateStreamResPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

/**
 * FMLE start publish: ReleaseStream/PublishStream/FCPublish/FCUnpublish
 */
class RtmpFMLEStartPacket: public RtmpPacket {
 public:
    /**
     * Name of the command
     */
    std::string command_name;
    /**
     * the transaction ID to get the response.
     */
    double transaction_id;
    /**
     * If there exists any command info this is set, else this is set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null
    /**
     * the stream name to start publish or release.
     */
    std::string stream_name;

 public:
    RtmpFMLEStartPacket();
    virtual ~RtmpFMLEStartPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
// factory method to create specified FMLE packet.

 public:
    static RtmpFMLEStartPacket* create_release_stream(std::string stream);
    static RtmpFMLEStartPacket* create_FC_publish(std::string stream);
};

/**
 * response for SrsFMLEStartPacket.
 */
class RtmpFMLEStartResPacket: public RtmpPacket {
 public:
    /**
     * Name of the command
     */
    std::string command_name;
    /**
     * the transaction ID to get the response.
     */
    double transaction_id;
    /**
     * If there exists any command info this is set, else this is set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null
    /**
     * the optional args, set to undefined.
     * @remark, never be NULL, an AMF0 undefined instance.
     */
    std::shared_ptr<Amf0Any> args;  // undefined

 public:
    explicit RtmpFMLEStartResPacket(double _transaction_id);
    virtual ~RtmpFMLEStartResPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

/**
 * FMLE/flash publish
 * 4.2.6. Publish
 * The client sends the publish command to publish a named stream to the
 * server. Using this name, any client can play this stream and receive
 * the published audio, video, and data messages.
 */
class RtmpPublishPacket: public RtmpPacket {
 public:
    /**
     * Name of the command, set to "publish".
     */
    std::string command_name;
    /**
     * Transaction ID set to 0.
     */
    double transaction_id;
    /**
     * Command information object does not exist. Set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null
    /**
     * Name with which the stream is published.
     */
    std::string stream_name;
    /**
     * Type of publishing. Set to "live", "record", or "append".
     *   record: The stream is published and the data is recorded to a new file.The file
     *           is stored on the server in a subdirectory within the directory that
     *           contains the server application. If the file already exists, it is
     *           overwritten.
     *   append: The stream is published and the data is appended to a file. If no file
     *           is found, it is created.
     *   live: Live data is published without recording it in a file.
     * @remark, TMSS only support live.
     * @remark, optional, default to live.
     */
    std::string type;

 public:
    RtmpPublishPacket();
    virtual ~RtmpPublishPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

/**
 * 4.2.8. pause
 * The client sends the pause command to tell the server to pause or
 * start playing.
 */
class RtmpPausePacket: public RtmpPacket {
 public:
    /**
     * Name of the command, set to "pause".
     */
    std::string command_name;
    /**
     * There is no transaction ID for this command. Set to 0.
     */
    double transaction_id;
    /**
     * Command information object does not exist. Set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null
    /**
     * true or false, to indicate pausing or resuming play
     */
    bool is_pause;
    /**
     * Number of milliseconds at which the the stream is paused or play resumed.
     * This is the current stream time at the Client when stream was paused. When the
     * playback is resumed, the server will only send messages with timestamps
     * greater than this value.
     */
    double time_ms;

 public:
    RtmpPausePacket();
    virtual ~RtmpPausePacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
};

/**
 * onStatus command, AMF0 Call
 * @remark, user must set the stream_id by SrsCommonMessage.set_packet().
 */
class RtmpOnStatusCallPacket: public RtmpPacket {
 public:
    /**
     * Name of command. Set to "onStatus"
     */
    std::string command_name;
    /**
     * Transaction ID set to 0.
     */
    double transaction_id;
    /**
     * Command information does not exist. Set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> args;  // null
    /**
     * Name-value pairs that describe the response from the server.
     * 'code','level', 'description' are names of few among such information.
     * @remark, never be NULL, an AMF0 object instance.
     */
    std::shared_ptr<Amf0Object> data;
    // renyu: onStatus的code内容
    std::string on_status_str;

 public:
    RtmpOnStatusCallPacket();
    virtual ~RtmpOnStatusCallPacket();

 public:
    virtual int decode(Buffer* stream);      // renyu: 增加解析onStatus命令字方法，回源下发的可以定制化处理
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

/**
 * the stream metadata.
 * FMLE: @setDataFrame
 * others: onMetaData
 */
class RtmpOnMetaDataPacket: public RtmpPacket {
 public:
    /**
     * Name of metadata. Set to "onMetaData"
     */
    std::string name;
    /**
     * Metadata of stream.
     * @remark, never be NULL, an AMF0 object instance.
     */
    std::shared_ptr<Amf0Object> metadata;

 public:
    RtmpOnMetaDataPacket();
    virtual ~RtmpOnMetaDataPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);

 public:
    static void fill_spaces(std::stringstream& ss, int level);

 public:
    static void get_body(std::shared_ptr<Amf0Any> any, std::stringstream& ss, int level);
};

/**
 * the special packet for the bandwidth test.
 * actually, it's a SrsOnStatusCallPacket, but
 * 1. encode with data field, to send data to client.
 * 2. decode ignore the data field, donot care.
 */
class RtmpBandwidthPacket: public RtmpPacket {
 public:
    /**
     * Name of command.
     */
    std::string command_name;
    /**
     * Transaction ID set to 0.
     */
    double transaction_id;
    /**
     * Command information does not exist. Set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> args;  // null
    /**
     * Name-value pairs that describe the response from the server.
     * 'code','level', 'description' are names of few among such information.
     * @remark, never be NULL, an AMF0 object instance.
     */
    std::shared_ptr<Amf0Object> data;

 public:
    RtmpBandwidthPacket();
    virtual ~RtmpBandwidthPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
// help function for bandwidth packet.

 public:
    virtual bool is_start_play();
    virtual bool is_starting_play();
    virtual bool is_stop_play();
    virtual bool is_stopped_play();
    virtual bool is_start_publish();
    virtual bool is_starting_publish();
    virtual bool is_stop_publish();
    virtual bool is_stopped_publish();
    virtual bool is_finish();
    virtual bool is_final();
    static RtmpBandwidthPacket* create_start_play();
    static RtmpBandwidthPacket* create_starting_play();
    static RtmpBandwidthPacket* create_playing();
    static RtmpBandwidthPacket* create_stop_play();
    static RtmpBandwidthPacket* create_stopped_play();
    static RtmpBandwidthPacket* create_start_publish();
    static RtmpBandwidthPacket* create_starting_publish();
    static RtmpBandwidthPacket* create_publishing();
    static RtmpBandwidthPacket* create_stop_publish();
    static RtmpBandwidthPacket* create_stopped_publish();
    static RtmpBandwidthPacket* create_finish();
    static RtmpBandwidthPacket* create_final();

 private:
    virtual RtmpBandwidthPacket* set_command(std::string command);
};

/**
 * client close stream packet.
 */
class RtmpCloseStreamPacket: public RtmpPacket {
 public:
    /**
     * Name of the command, set to "closeStream".
     */
    std::string command_name;
    /**
     * Transaction ID set to 0.
     */
    double transaction_id;
    /**
     * Command information object does not exist. Set to null type.
     * @remark, never be NULL, an AMF0 null instance.
     */
    std::shared_ptr<Amf0Any> command_object;  // null

 public:
    RtmpCloseStreamPacket();
    virtual ~RtmpCloseStreamPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
};

/**
 * 4.1.2. Call
 * The call method of the NetConnection object runs remote procedure
 * calls (RPC) at the receiving end. The called RPC name is passed as a
 * parameter to the call command.
 */
class RtmpCallPacket: public RtmpPacket {
 public:
    /**
     * Name of the remote procedure that is called.
     */
    std::string command_name;
    /**
     * If a response is expected we give a transaction Id. Else we pass a value of 0
     */
    double transaction_id;
    /**
     * If there exists any command info this
     * is set, else this is set to null type.
     * @remark, optional, init to and maybe NULL.
     */
    std::shared_ptr<Amf0Any> command_object;
    /**
     * Any optional arguments to be provided
     * @remark, optional, init to and maybe NULL.
     */
    std::shared_ptr<Amf0Any> arguments;

 public:
    RtmpCallPacket();
    virtual ~RtmpCallPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};
/**
 * response for RtmpCallPacket.
 */
class RtmpCallResPacket: public RtmpPacket {
 public:
    /**
     * Name of the command.
     */
    std::string command_name;
    /**
     * ID of the command, to which the response belongs to
     */
    double transaction_id;
    /**
     * If there exists any command info this is set, else this is set to null type.
     * @remark, optional, init to and maybe NULL.
     */
    std::shared_ptr<Amf0Any> command_object;
    /**
     * Response from the method that was called.
     * @remark, optional, init to and maybe NULL.
     */
    std::shared_ptr<Amf0Any> response;

 public:
    explicit RtmpCallResPacket(double _transaction_id);
    virtual ~RtmpCallResPacket();
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

class RtmpOnCustomAmfDataPacket: public RtmpPacket {
 public:
    // renyu: AMF数据的名称，不是固定的，需要解析AMF数据的
    std::string name;

 public:
    RtmpOnCustomAmfDataPacket();
    virtual ~RtmpOnCustomAmfDataPacket();

 public:
    virtual int decode(Buffer* stream);
};

/**
 * 5.3. Acknowledgement (3)
 * The client or the server sends the acknowledgment to the peer after
 * receiving bytes equal to the window size.
 */
class RtmpAcknowledgementPacket: public RtmpPacket {
 public:
    uint32_t sequence_number;

 public:
    RtmpAcknowledgementPacket();
    virtual ~RtmpAcknowledgementPacket();
// decode functions for concrete packet to override.

 public:
    virtual int decode(Buffer* stream);
// encode functions for concrete packet to override.

 public:
    virtual int get_prefer_cid();
    virtual int get_message_type();

 protected:
    virtual int get_size();
    virtual int encode_packet(Buffer* stream);
};

class AckWindowSize {
 public:
    uint32_t window;
    // number of received bytes.
    int64_t nb_recv_bytes;
    // previous responsed sequence number.
    uint32_t sequence_number;

    AckWindowSize();
};
class RtmpProtocolHandler {
 public:
     explicit RtmpProtocolHandler(std::shared_ptr<IConn> io, bool fix_timestamp = false);
     virtual ~RtmpProtocolHandler();

 public:
     virtual int send_packet(std::shared_ptr<RtmpPacket> pkt, int streamid);

     /*
     * receive and decode a rtmp packet
     */
     virtual int recv_packet(std::shared_ptr<RtmpPacket>& pkt);

     virtual int recv_message(char* buf, int size);

     virtual int send_message(const char* buf, int size);

     template<class T>
     int expect_packet(std::shared_ptr<T>& packet) {
        int ret = error_success;

        while (true) {
            std::shared_ptr<RtmpPacket> rtmp_packet;
            if ((ret = recv_packet(rtmp_packet)) != error_success) {
                tmss_error("receive packet failed, ret={}", ret);
                return ret;
            }

            std::shared_ptr<T> pkt = std::dynamic_pointer_cast<T>(rtmp_packet);
            if (!pkt) {
                tmss_info("the packet type invalid");
                continue;
            }
            packet = pkt;
            break;
        }
        return ret;
    }

 public:
    /**
     * recv bytes oriented RTMP message from protocol stack.
     * recv_buf is the buffer need write, if it is null, then need create_buffer
     * return error if error occur and nerver set the pmsg,
     * return success and pmsg set to NULL if no entire message got,
     * return success and pmsg set to entire message if got one.
     */
    virtual int recv_interlaced_message(std::shared_ptr<RtmpMessage>& msg,
        char* recv_buf = nullptr, int buf_size = 0);
    /**
     * read the chunk basic header(fmt, cid) from chunk stream.
     * user can discovery a RtmpChunkStream by cid.
     */
    virtual int read_basic_header(char& fmt, int& cid);
    /**
     * read the chunk message header(timestamp, payload_length, message_type, stream_id)
     * from chunk stream and save to RtmpChunkStream.
     */
    virtual int read_message_header(std::shared_ptr<RtmpChunkStream> chunk, char fmt);
    /**
     * read the chunk payload, remove the used bytes in buffer,
     * if got entire message, set the pmsg.
     */
    virtual int read_message_payload(std::shared_ptr<RtmpChunkStream> chunk,
            std::shared_ptr<RtmpMessage>& msg);

    virtual int on_recv_message(std::shared_ptr<RtmpMessage>& msg,
        std::shared_ptr<RtmpPacket>& packet);

    virtual int do_decode_message(MessageHeader& header, Buffer* stream,
            std::shared_ptr<RtmpPacket>& packet);

    virtual int do_decode_command(MessageHeader& header,
        const std::string& command, Buffer* stream,
        std::shared_ptr<RtmpPacket>& packet);
    /**
     * auto response the ack message.
     */
    virtual int response_acknowledgement_message();
    /**
     * auto response the ping message.
     */
    virtual int response_ping_message(int32_t timestamp);

 private:
    std::shared_ptr<IConn> conn;

    std::shared_ptr<IOBuffer> io_buffer;

    /**
     * chunk stream to decode RTMP messages.
     */
    std::map<int, std::shared_ptr<RtmpChunkStream>> chunk_streams;

    /**
     * cache some frequently used chunk header.
     * cs_cache, the chunk stream cache.
     * @see https://github.com/ossrs/srs/issues/249
     */
    std::vector<std::shared_ptr<RtmpChunkStream>> cs_cache;
    bool    fix_rtmp_timestamp;

    /**
     * input chunk size, default to 128, set by peer packet.
     */
    int32_t in_chunk_size;
    /**
     * output chunk size, default to 128, set by config.
     */
    int32_t out_chunk_size;

    /**
     * requests sent out, used to build the response.
     * key: transactionId
     * value: the request command name
     */
    std::map<double, std::string> requests;

    // The input ack window, to response acknowledge to peer,
    // for example, to respose the encoder, for server got lots of packets.
    AckWindowSize in_ack_size;
    // The output ack window, to require peer to response the ack.
    AckWindowSize out_ack_size;
    // The buffer length set by peer.
    int32_t in_buffer_length;

    /**
     * whether auto response when recv messages.
     * default to true for it's very easy to use the protocol stack.
     * @see: https://github.com/ossrs/srs/issues/217
     */
    bool auto_response_when_recv;
    /**
     * when not auto response message, manual flush the messages in queue.
     */
    std::vector<std::shared_ptr<RtmpPacket> > manual_response_queue;

    /**
     * cache for multiple messages send,
     * initialize to iovec[TMSS_CONSTS_IOVS_MAX] and realloc when consumed,
     * it's ok to realloc the iovs cache, for all ptr is ok.
     */
    iovec* out_iovs;
    int nb_out_iovs;
    /**
     * output header cache.
     * used for type0, 11bytes(or 15bytes with extended timestamp) header.
     * or for type3, 1bytes(or 5bytes with extended timestamp) header.
     * the c0c3 caches must use unit TMSS_CONSTS_RTMP_MAX_FMT0_HEADER_SIZE bytes.
     *
     * @remark, the c0c3 cache cannot be realloc.
     */
    char out_c0c3_caches[CONSTS_C0C3_HEADERS_MAX];
    // whether warned user to increase the c0c3 header cache.
    bool warned_c0c3_cache_dry;
};

// 3.7. User Control message
enum SrcPCUCEventType {
    // generally, 4bytes event-data

    /**
     * The server sends this event to notify the client
     * that a stream has become functional and can be
     * used for communication. By default, this event
     * is sent on ID 0 after the application connect
     * command is successfully received from the
     * client. The event data is 4-byte and represents
     * the stream ID of the stream that became
     * functional.
     */
    SrcPCUCStreamBegin = 0x00,

    /**
     * The server sends this event to notify the client
     * that the playback of data is over as requested
     * on this stream. No more data is sent without
     * issuing additional commands. The client discards
     * the messages received for the stream. The
     * 4 bytes of event data represent the ID of the
     * stream on which playback has ended.
     */
     SrcPCUCStreamEOF = 0x01,

     /**
      * The server sends this event to notify the client
      * that there is no more data on the stream. If the
      * server does not detect any message for a time
      * period, it can notify the subscribed clients
      * that the stream is dry. The 4 bytes of event
      * data represent the stream ID of the dry stream.
      */
      SrcPCUCStreamDry = 0x02,

      /**
       * The client sends this event to inform the server
       * of the buffer size (in milliseconds) that is
       * used to buffer any data coming over a stream.
       * This event is sent before the server starts
       * processing the stream. The first 4 bytes of the
       * event data represent the stream ID and the next
       * 4 bytes represent the buffer length, in
       * milliseconds.
       */
       SrcPCUCSetBufferLength = 0x03,  // 8bytes event-data

       /**
        * The server sends this event to notify the client
        * that the stream is a recorded stream. The
        * 4 bytes event data represent the stream ID of
        * the recorded stream.
        */
        SrcPCUCStreamIsRecorded = 0x04,

        /**
         * The server sends this event to test whether the
         * client is reachable. Event data is a 4-byte
         * timestamp, representing the local server time
         * when the server dispatched the command. The
         * client responds with kMsgPingResponse on
         * receiving kMsgPingRequest.
         */
         SrcPCUCPingRequest = 0x06,

         /**
          * The client sends this event to the server in
          * response to the ping request. The event data is
          * a 4-byte timestamp, which was received with the
          * kMsgPingRequest request.
          */
          SrcPCUCPingResponse = 0x07,

          /**
           * for PCUC size=3, the payload is "00 1A 01",
           * where we think the event is 0x001a, fms defined msg,
           * which has only 1bytes event data.
           */
           SrcPCUCFmsEvent0 = 0x1a,
};

extern int chunk_header_c0(int perfer_cid, uint32_t timestamp, int32_t payload_length,
    int8_t message_type, int32_t stream_id,
    char* cache, int nb_cache);

extern int chunk_header_c3(int perfer_cid, uint32_t timestamp,
    char* cache, int nb_cache);
}  // namespace tmss
