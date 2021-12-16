/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/29.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */
#pragma once

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <cstdint>

typedef int error_t;
typedef uint64_t utime_t;

#define error_success 0L

//  system, io
#define error_system_undefined 10000
#define error_system_handler_not_found 10001
#define error_socket_create     10002
#define error_socket_open     10003
#define error_socket_read     10004
#define error_socket_write     10005
#define error_socket_bind     10006
#define error_socket_listen    10007
#define error_socket_timeout    10008
#define error_socket_already_closed 10009
#define error_socket_set_resuse    10010
#define error_socket_connect     10011
#define error_system_ip_invalid 10012
#define error_cothread_start    10020
#define error_cothread_interrupt 10021
#define error_cothread_stop     10022
#define error_srt_socket_create     10102
#define error_srt_socket_open     10103
#define error_srt_socket_read     10104
#define error_srt_socket_write     10105
#define error_srt_socket_bind     10106
#define error_srt_socket_listen    10107
#define error_srt_socket_timeout    10108
#define error_srt_socket_already_closed 10109
#define error_srt_socket_set_resuse    10110
#define error_srt_socket_connect     10111
#define error_srt_socket_broken      10112
#define error_srt_socket_closing     10113
#define error_srt_socket_noexist     10114
#define error_srt_socket_other       10115
#define error_srt_socket_eintr       10116
#define error_config_load            10500

//  cache
#define error_channel_killed  11001
#define error_buffer_read       12001
#define error_buffer_write       12002
#define error_buffer_not_enough  12003
#define error_ingest_no_input    13001
#define error_ingest_no_client   13002

// protocol
#define error_rtmp_complex_handshake_not_support 14001
#define error_rtmp_handshake    14002
#define error_rtmp_amf0_decode  14100
#define error_rtmp_amf0_encode  14101
#define error_rtmp_packet_invalid   14102
#define error_rtmp_no_request 14103
#define error_rtmp_chunk_start 14104
#define error_rtmp_packet_size 14105
#define error_rtmp_amf0_invalid 14106
#define error_rtmp_message_encode 14107
#define error_rtmp_chunk_size 14108
#define error_tag_type_invalid 14200
#define error_rtmp_message_decode 14201

//  file
#define error_file_buffer_not_enough    16001
#define error_file_read_not_complete    16002
#define error_file_buffer_init_small    16003

//  ffmpeg
#define error_ffmpeg_init_input         17001
#define error_ffmpeg_read               17002
#define error_ffmpeg_init_output        17003
#define error_ffmpeg_write              17004
#define error_ffmpeg_write_header       17005