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

#include "util/util.hpp"
#include <defs/err.hpp>

/**
* this file defines the perfromance options.
*/

/**
* to improve read performance, merge some packets then read,
* when it on and read small bytes, we sleep to wait more data.,
* that is, we merge some data to read together.
* @see SrsConfig::get_mr_enabled()
* @see SrsConfig::get_mr_sleep_ms()
* @see https://github.com/ossrs/srs/issues/241
* @example, for the default settings, this algorithm will use:
*       that is, when got nread bytes smaller than 4KB, sleep(780ms).
*/

/**
* the MW(merged-write) send cache time in ms.
* the default value, user can override it in config.
* to improve send performance, cache msgs and send in a time.
* for example, cache 500ms videos and audios, then convert all these
* msgs to iovecs, finally use writev to send.
* @remark this largely improve performance, from 3.5k+ to 7.5k+.
*       the latency+ when cache+.
* @remark the socket send buffer default to 185KB, it large enough.
* @see https://github.com/ossrs/srs/issues/194
* @see SrsConfig::get_mw_sleep_ms()
* @remark the mw sleep and msgs to send, maybe:
*       mw_sleep        msgs        iovs
*       350             43          86
*       400             44          88
*       500             46          92
*       600             46          92
*       700             82          164
*       800             81          162
*       900             80          160
*       1000            88          176
*       1100            91          182
*       1200            89          178
*       1300            119         238
*       1400            120         240
*       1500            119         238
*       1600            131         262
*       1700            131         262
*       1800            133         266
*       1900            141         282
*       2000            150         300
*/
// the default config of mw.
#define TMSS_PERF_MW_SLEEP 350
/**
* how many msgs can be send entirely.
* for play clients to get msgs then totally send out.
* for the mw sleep set to 1800, the msgs is about 133.
* @remark, recomment to 128.
*/
#define TMSS_PERF_MW_MSGS 128

/**
* whether set the socket send buffer size.
* @see https://github.com/ossrs/srs/issues/251
*/
#define TMSS_PERF_MW_SO_SNDBUF

/**
* whether set the socket recv buffer size.
* @see https://github.com/ossrs/srs/issues/251
*/
#undef TMSS_PERF_MW_SO_RCVBUF
/**
* whether enable the fast vector for qeueue.
* @see https://github.com/ossrs/srs/issues/251
*/
#define TMSS_PERF_QUEUE_FAST_VECTOR
/**
* whether use cond wait to send messages.
* @remark this improve performance for large connectios.
* @see https://github.com/ossrs/srs/issues/251
*/
#define TMSS_PERF_QUEUE_COND_WAIT
#ifdef TMSS_PERF_QUEUE_COND_WAIT
    #define TMSS_PERF_MW_MIN_MSGS 8
#endif
/**
* the default value of vhost for
* TMSS whether use the min latency mode.
* for min latence mode:
* 1. disable the mr for vhost.
* 2. use timeout for cond wait for consumer queue.
* @see https://github.com/ossrs/srs/issues/257
*/
#define TMSS_PERF_MIN_LATENCY_ENABLED false

/**
* how many chunk stream to cache, [0, N].
* to imporove about 10% performance when chunk size small, and 5% for large chunk.
* @see https://github.com/ossrs/srs/issues/249
* @remark 0 to disable the chunk stream cache.
*/
#define TMSS_PERF_CHUNK_STREAM_CACHE 16

/**
* the gop cache and play cache queue.
*/
// whether gop cache is on.
#define TMSS_PERF_GOP_CACHE true
// in seconds, the live queue length.
#define TMSS_PERF_PLAY_QUEUE 30

#define TMSS_MIN_QUEUE_SIZE_COUNT 2000
/**
* whether always use complex send algorithm.
* for some network does not support the complex send,
* @see https://github.com/ossrs/srs/issues/320
*/
//  #undef TMSS_PERF_COMPLEX_SEND
#define TMSS_PERF_COMPLEX_SEND
/**
 * whether enable the TCP_NODELAY
 * user maybe need send small tcp packet for some network.
 * @see https://github.com/ossrs/srs/issues/320
 */
#undef TMSS_PERF_TCP_NODELAY
#define TMSS_PERF_TCP_NODELAY
/**
* set the socket send buffer,
* to force the server to send smaller tcp packet.
* @see https://github.com/ossrs/srs/issues/320
* @remark undef it to auto calc it by merged write sleep ms.
* @remark only apply it when TMSS_PERF_MW_SO_SNDBUF is defined.
*/
#ifdef TMSS_PERF_MW_SO_SNDBUF
    //  #define TMSS_PERF_SO_SNDBUF_SIZE 1024
    #undef TMSS_PERF_SO_SNDBUF_SIZE
#endif

/**
 * define the following macro to enable the fast flv encoder.
 * @see https://github.com/ossrs/srs/issues/405
 */
#undef TMSS_PERF_FAST_FLV_ENCODER
#define TMSS_PERF_FAST_FLV_ENCODER

/**
* E.4.1 FLV Tag, page 75
*/
enum CodecFlvTag {
    // set to the zero to reserved, for array map.
    CodecFlvTagReserved = 0,

    // 8 = audio
    CodecFlvTagAudio = 8,
    // 9 = video
    CodecFlvTagVideo = 9,
    // 18 = script data
    CodecFlvTagScript = 18,
};

/**
 * 6.1.2. Chunk Message Header
 * There are four different formats for the chunk message header,
 * selected by the "fmt" field in the chunk basic header.
 */
// 6.1.2.1. Type 0
// Chunks of Type 0 are 11 bytes long. This type MUST be used at the
// start of a chunk stream, and whenever the stream timestamp goes
// backward (e.g., because of a backward seek).
#define RTMP_FMT_TYPE0                          0
// 6.1.2.2. Type 1
// Chunks of Type 1 are 7 bytes long. The message stream ID is not
// included; this chunk takes the same stream ID as the preceding chunk.
// Streams with variable-sized messages (for example, many video
// formats) SHOULD use this format for the first chunk of each new
// message after the first.
#define RTMP_FMT_TYPE1                          1
// 6.1.2.3. Type 2
// Chunks of Type 2 are 3 bytes long. Neither the stream ID nor the
// message length is included; this chunk has the same stream ID and
// message length as the preceding chunk. Streams with constant-sized
// messages (for example, some audio and data formats) SHOULD use this
// format for the first chunk of each message after the first.
#define RTMP_FMT_TYPE2                          2
// 6.1.2.4. Type 3
// Chunks of Type 3 have no header. Stream ID, message length and
// timestamp delta are not present; chunks of this type take values from
// the preceding chunk. When a single message is split into chunks, all
// chunks of a message except the first one, SHOULD use this type. Refer
// to example 2 in section 6.2.2. Stream consisting of messages of
// exactly the same size, stream ID and spacing in time SHOULD use this
// type for all chunks after chunk of Type 2. Refer to example 1 in
// section 6.2.1. If the delta between the first message and the second
// message is same as the time stamp of first message, then chunk of
// type 3 would immediately follow the chunk of type 0 as there is no
// need for a chunk of type 2 to register the delta. If Type 3 chunk
// follows a Type 0 chunk, then timestamp delta for this Type 3 chunk is
// the same as the timestamp of Type 0 chunk.
#define RTMP_FMT_TYPE3                          3
/**
 5. Protocol Control Messages
 RTMP reserves message type IDs 1-7 for protocol control messages.
 These messages contain information needed by the RTM Chunk Stream
 protocol or RTMP itself. Protocol messages with IDs 1 & 2 are
 reserved for usage with RTM Chunk Stream protocol. Protocol messages
 with IDs 3-6 are reserved for usage of RTMP. Protocol message with ID
 7 is used between edge server and origin server.
 */
#define RTMP_MSG_SetChunkSize                   0x01
#define RTMP_MSG_AbortMessage                   0x02
#define RTMP_MSG_Acknowledgement                0x03
#define RTMP_MSG_UserControlMessage             0x04
#define RTMP_MSG_WindowAcknowledgementSize      0x05
#define RTMP_MSG_SetPeerBandwidth               0x06
#define RTMP_MSG_EdgeAndOriginServerCommand     0x07

/**
3. Types of messages
The server and the client send messages over the network to
communicate with each other. The messages can be of any type which
includes audio messages, video messages, command messages, shared
object messages, data messages, and user control messages.
3.1. Command message
Command messages carry the AMF-encoded commands between the client
and the server. These messages have been assigned message type value
of 20 for AMF0 encoding and message type value of 17 for AMF3
encoding. These messages are sent to perform some operations like
connect, createStream, publish, play, pause on the peer. Command
messages like onstatus, result etc. are used to inform the sender
about the status of the requested commands. A command message
consists of command name, transaction ID, and command object that
contains related parameters. A client or a server can request Remote
Procedure Calls (RPC) over streams that are communicated using the
command messages to the peer.
*/
#define RTMP_MSG_AMF3CommandMessage             17  // 0x11
#define RTMP_MSG_AMF0CommandMessage             20  // 0x14
/**
3.2. Data message
The client or the server sends this message to send Metadata or any
user data to the peer. Metadata includes details about the
data(audio, video etc.) like creation time, duration, theme and so
on. These messages have been assigned message type value of 18 for
AMF0 and message type value of 15 for AMF3.
*/
#define RTMP_MSG_AMF0DataMessage                18  // 0x12
#define RTMP_MSG_AMF3DataMessage                15  // 0x0F
/**
3.3. Shared object message
A shared object is a Flash object (a collection of name value pairs)
that are in synchronization across multiple clients, instances, and
so on. The message types kMsgContainer=19 for AMF0 and
kMsgContainerEx=16 for AMF3 are reserved for shared object events.
Each message can contain multiple events.
*/
#define RTMP_MSG_AMF3SharedObject               16  // 0x10
#define RTMP_MSG_AMF0SharedObject               19  // 0x13
/**
3.4. Audio message
The client or the server sends this message to send audio data to the
peer. The message type value of 8 is reserved for audio messages.
*/
#define RTMP_MSG_AudioMessage                   8  // 0x08
/* *
3.5. Video message
The client or the server sends this message to send video data to the
peer. The message type value of 9 is reserved for video messages.
These messages are large and can delay the sending of other type of
messages. To avoid such a situation, the video message is assigned
the lowest priority.
*/
#define RTMP_MSG_VideoMessage                   9  // 0x09
/**
3.6. Aggregate message
An aggregate message is a single message that contains a list of submessages.
The message type value of 22 is reserved for aggregate
messages.
*/
#define RTMP_MSG_AggregateMessage               22  // 0x16

/****************************************************************************
*****************************************************************************
****************************************************************************/
/**
* the chunk stream id used for some under-layer message,
* for example, the PC(protocol control) message.
*/
#define RTMP_CID_ProtocolControl                0x02
/**
* the AMF0/AMF3 command message, invoke method and return the result, over NetConnection.
* generally use 0x03.
*/
#define RTMP_CID_OverConnection                 0x03
/**
* the AMF0/AMF3 command message, invoke method and return the result, over NetConnection,
* the midst state(we guess).
* rarely used, e.g. onStatus(NetStream.Play.Reset).
*/
#define RTMP_CID_OverConnection2                0x04
/**
* the stream message(amf0/amf3), over NetStream.
* generally use 0x05.
*/
#define RTMP_CID_OverStream                     0x05
/**
* the stream message(amf0/amf3), over NetStream, the midst state(we guess).
* rarely used, e.g. play("mp4:mystram.f4v")
*/
#define RTMP_CID_OverStream2                    0x08
/**
* the stream message(video), over NetStream
* generally use 0x06.
*/
#define RTMP_CID_Video                          0x06
/**
* the stream message(audio), over NetStream.
* generally use 0x07.
*/
#define RTMP_CID_Audio                          0x07
// the default chunk size for system.
#define TMSS_CONSTS_RTMP_TMSS_CHUNK_SIZE         4096

// 6. Chunking, RTMP protocol default chunk size.
#define TMSS_CONSTS_RTMP_PROTOCOL_CHUNK_SIZE 128

/**
* 6. Chunking
* The chunk size is configurable. It can be set using a control
* message(Set Chunk Size) as described in section 7.1. The maximum
* chunk size can be 65536 bytes and minimum 128 bytes. Larger values
* reduce CPU usage, but also commit to larger writes that can delay
* other content on lower bandwidth connections. Smaller chunks are not
* good for high-bit rate streaming. Chunk size is maintained
* independently for each direction.
*/
#define TMSS_CONSTS_RTMP_MIN_CHUNK_SIZE 128
#define TMSS_CONSTS_RTMP_MAX_CHUNK_SIZE 65536

/**
* max rtmp header size:
*     1bytes basic header,
*     11bytes message header,
*     4bytes timestamp header,
* that is, 1+11+4=16bytes.
*/
#define TMSS_CONSTS_RTMP_MAX_FMT0_HEADER_SIZE 16
/**
* max rtmp header size:
*     1bytes basic header,
*     4bytes timestamp header,
* that is, 1+4=5bytes.
*/
// always use fmt0 as cache.
#define TMSS_CONSTS_RTMP_MAX_FMT3_HEADER_SIZE 5

#define RTMP_EXTENDED_TIMESTAMP                 0xFFFFFF

/**
* how many msgs can be send entirely.
* for play clients to get msgs then totally send out.
* for the mw sleep set to 1800, the msgs is about 133.
* @remark, recomment to 128.
*/
#define PERF_MW_MSGS 128

/**
* for performance issue, 
* the iovs cache, @see https://github.com/ossrs/srs/issues/194
* iovs cache for multiple messages for each connections.
* suppose the chunk size is 64k, each message send in a chunk which needs only 2 iovec,
* so the iovs max should be (TMSS_PERF_MW_MSGS * 2)
*
* @remark, will realloc when the iovs not enough.
*/
#define CONSTS_IOVS_MAX (PERF_MW_MSGS * 2)
/**
* for performance issue, 
* the c0c3 cache, @see https://github.com/ossrs/srs/issues/194
* c0c3 cache for multiple messages for each connections.
* each c0 <= 16byes, suppose the chunk size is 64k,
* each message send in a chunk which needs only a c0 header,
* so the c0c3 cache should be (TMSS_PERF_MW_MSGS * 16)
*
* @remark, TMSS will try another loop when c0c3 cache dry, for we cannot realloc it.
*       so we use larger c0c3 cache, that is (TMSS_PERF_MW_MSGS * 32)
*/
#define CONSTS_C0C3_HEADERS_MAX (PERF_MW_MSGS * 32)

//  flv header
#define FLV_MSG_HeaderMessage                   0x61

