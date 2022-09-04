/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/26.
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#include <string>

#include <media_source_handler.hpp>
#include "http_server.hpp"
#include <format/raw/tmss_format_raw.hpp>
#include <format/ffmpeg/tmss_format_base.hpp>
#include <transport/tmss_trans_tcp.hpp>
#include <log/log.hpp>
#include <util/timer.hpp>
#include <protocol/http/http_client.hpp>
#include <protocol/rtmp/rtmp_client.hpp>

namespace tmss {
int MediaSource::handle_connect(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    return ret;
}

int MediaSource::handle_request(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;

    switch (req->type) {
        case ERequestTypePlay: {
            ret = handle_play(conn, req, server);
            break;
        }
        case ERequestTypePublish: {
            ret = handle_publish(conn, req, server);
            break;
        }
        case ERequestTypeApi: {
            ret = handle_api(conn, req, server);
            break;
        }
        default: {
            ret = handle_play(conn, req, server);
            break;
        }
    }
    return ret;
}

int MediaSource::handle_cycle(std::shared_ptr<Channel> channel) {
    int ret = error_success;
    while (true) {
        /* time expire, here vary from different logic */
        if (channel->get_idle_time() > 5 * 1000 * 1000) {
            tmss_info("channel idle complete, break");
            break;
        }
        std::vector<std::shared_ptr<IPacket> > packet_group;
        // get packets from input, one packet per input
        channel->fetch(packet_group, 10 * 1000 * 1000);

        if (packet_group.empty()) {
            // no input
            tmss_info("no packet");
            break;
        }

        tmss_info("read a packet");
        // suppose there is only one input, to do
        std::shared_ptr<IPacket> packet = packet_group[0];

        // codec, filter
        // to do
        #if 0
        sorted<IPacket> cache = channel->get_cache();

        if (/*this is metadata*/) {
            cache[METADATA] = packet;
        } else if (/*this is avc_header*/) {
            cache[VIDEO_HEADER] = packet;
        } else if (/*this is aac_header*/) {
            cache[AUDIO_HEADER] = packet;
        }
        #endif
        if (packet) {
            channel->dispatch(packet);
            channel->on_cycle(packet);
        }
    }
    return ret;
}

int MediaSource::handle_disconnect(std::shared_ptr<IClientConn> conn) {
    int ret = error_success;

    return ret;
}

MediaSource::MediaSource() {
}

MediaSource::~MediaSource() {
    for (auto iter = server_group.begin(); iter != server_group.end(); iter++) {
        // to do
        server_group.erase(iter);
    }
}

int parse_params(int num, char** param, int &port) {
    std::string temp;
    for (int i = 1; i < num; i++) {
        char* p = param[i];
        if (*p) {
            tmss_info("{}param={}", i, *p);
        } else {
            tmss_info("{}param=null", i);
        }
        if (*p++ != '-') {
            return -1;
        }
        while (*p) {
            if (*p) {
                tmss_info("{}param={}", i, *p);
            } else {
                tmss_info("{}param=null", i);
            }
            switch (*p++) {
                case 'p':
                case 'P':
                    if (*p) {
                        temp = p;
                        tmss_info("temp={}", temp.c_str());
                        continue;
                    }
                    if (param[++i]) {
                        temp = param[i];
                        continue;
                    }
                    return -1;
                default:
                    break;
            }
        }
    }
    if (!temp.empty()) {
        tmss_info("port={}", temp.c_str());
        port = atoll(temp.c_str());
    }
    return error_success;
}

int MediaSource::init(int num, char** param) {
    int ret = error_success;
    std::string ip = "127.0.0.1";
    int port = 8002;
    tmss_info("there are {} params", num);
    parse_params(num, param, port);
    // load config
    // different server can share the same channel or file
    std::shared_ptr<ChannelPool> channel_pool = std::make_shared<ChannelPool>();
    channel_pool->start();
    std::shared_ptr<FileCache> file_cache = std::make_shared<FileCache>();

    // http server over tcp
    auto server_conn = std::make_shared<TcpServerConn>();
    std::shared_ptr<IServer> server = std::make_shared<HttpServer>(server_conn, channel_pool, file_cache);
    ret = server->init(ip, port);
    if (ret != error_success) {
        tmss_error("http_server init failed, ret={}", ret);
        return ret;
    }
    ret = server->run();
    if (ret != error_success) {
        tmss_error("http_server run failed, ret={}", ret);
        return ret;
    }

    server_group.push_back(server);
    // other servers

    tmss_info("init success");
    return ret;
}

int MediaSource::handle_play(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    // check config, to do

    //  to do
    req->format = req->ext;

    //  test
    std::string mode = req->params_map["mode"];
    if (mode == "hls") {
        return handle_play_segment(conn, req, server);
    } else if (mode == "live") {
        return handle_play_stream(conn, req, server);
    } else {
        return handle_play_file(conn, req, server);
    }
}

int MediaSource::handle_play_stream(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;
    // get or create Channel
    std::string stream_key = create_channel_key(req);
    tmss_info("handle_play_stream,stream_key={}", stream_key.c_str());
    std::shared_ptr<Channel> channel;
    std::shared_ptr<ChannelMgr> channel_mgr = server->get_channel_mgr();
    channel_mgr->fetch_or_create_channel(stream_key, channel);
    channel->init_user_ctrl(shared_from_this());
    // get or create output
    std::shared_ptr<Pool<OutputHandler>> output_pool = server->get_output_pool();
    std::shared_ptr<OutputHandler> output = std::make_shared<OutputHandler>(output_pool, channel);
    output->init_conn(conn);

    // create mux based on request
    std::shared_ptr<IMux> muxer = create_mux_by_ext(req->ext);
    std::shared_ptr<IContext> context = create_context_by_ext(req->ext);
    output->init_format(muxer);
    std::shared_ptr<HttpClient> client = std::make_shared<HttpClient>();
    client->init(conn);
    output->init_play_client(client);
    output->set_context(context);

    output->set_type(EOutputPlay);

    channel->add_output(output);

    tmss_info("handle_play_stream");

    if (channel->get_status() == EChannelStart) {
        tmss_info("channel is already running");
    } else {
        // check if it need origin
        create_origin_stream(channel, req, server);

        // start channel
        ret = channel->run();
        if (ret != error_success) {
            tmss_error("channel run failed,ret={}", ret);
            channel->on_stop();
            //  return ret;
        }
    }

    output->run();

    tmss_info("handle_play_stream");

    return ret;
}

int MediaSource::handle_publish(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;
    // get or create Channel
    std::string stream_key = create_channel_key(req);

    std::shared_ptr<Channel> channel;
    std::shared_ptr<ChannelMgr> channel_mgr = server->get_channel_mgr();
    channel_mgr->fetch_or_create_channel(stream_key, channel);
    channel->init_user_ctrl(shared_from_this());

    std::shared_ptr<Pool<InputHandler>> input_pool = server->get_input_pool();
    std::shared_ptr<InputHandler> input = std::make_shared<InputHandler>(input_pool, channel);

    // check req->format
    std::shared_ptr<IDeMux> demuxer = std::make_shared<RawDeMux>();
    input->init_format(demuxer);
    input->init_conn(conn);

    std::shared_ptr<IContext> context = std::make_shared<IContext>();
    input->set_context(context);
    input->set_type(EInputPublish);

    channel->add_input(input);

    input->run();

    // check if it need forward
    create_forward(channel, req, server);

    // start channel
    channel->run();

    return ret;
}

int MediaSource::handle_api(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;

    std::string stream_key = create_channel_key(req);

    std::shared_ptr<Channel> channel;
    std::shared_ptr<ChannelMgr> channel_mgr = server->get_channel_mgr();
    channel_mgr->fetch_or_create_channel(stream_key, channel);
    channel->init_user_ctrl(shared_from_this());

    create_origin_stream(channel, req, server);

    // get or create output
    create_forward(channel, req, server);

    // start channel
    channel->run();

    return ret;
}

std::string MediaSource::create_channel_key(std::shared_ptr<Request> req) {
    return req->vhost + "|" + req->path + "|" + req->name;
}

int MediaSource::create_origin_stream(std::shared_ptr<Channel> channel,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;
    // get or create input
    std::string origin_host = req->vhost;   //  to do
    std::string origin_path = req->path;
    std::string stream = req->name;
    //  parse streamid and ext
    if (true) {
        std::size_t found = stream.find(".");
        if (found == std::string::npos) {
        } else {
            stream = stream.substr(0, found);
        }
    }
    std::string param = req->params;
    // connect to origin
    std::shared_ptr<IClientConn> origin_conn = std::make_shared<TcpStreamConn>(nullptr);

    std::shared_ptr<Pool<InputHandler>> input_pool = server->get_input_pool();
    std::shared_ptr<InputHandler> input = std::make_shared<InputHandler>(input_pool, channel);

    // check req->format
    std::string origin_format = "flv";    //  to do
    std::shared_ptr<IContext> context = create_context_by_format(origin_format);
    std::shared_ptr<IDeMux> demuxer = create_demux_by_format(origin_format);

    std::shared_ptr<HttpClient> origin_client = std::make_shared<HttpClient>();
    origin_client->init(origin_conn);
    input->init_format(demuxer);
    input->init_conn(origin_conn);
    input->init_origin_client(origin_client);
    input->set_context(context);
    input->set_type(EInputOrigin);

    std::string origin_ip = origin_host;     //  to do
    if (req->params_map["origin_host"].length() > 0) {
        origin_ip = req->params_map["origin_host"];
    }
    int origin_port = 80;
    if (req->params_map["origin_port"].length() > 0) {
        origin_port = atoll(req->params_map["origin_port"].c_str());
    }
    if (req->params_map["origin_param"] == "no") {
        param = "";
    }
    tmss_info("origin_host={}, origin_ip={}, origin_port={}", origin_ip, origin_ip, origin_port);
    Address address(origin_ip, origin_port);
    //  input->set_origin_address(address);
    input->set_origin_info(address, origin_host, origin_path, stream, origin_format, param);

    channel->add_input(input);

    // input->run();
    // to do
    // input maybe is other channels
    #if 0
    std::shared_ptr<Channel> other_channel;
    std::shared_ptr<PacketQueue> queue;  // new queue
    channel->add_input(queue);
    other_channel->add_output(queue);
    #endif

    return ret;
}

int MediaSource::create_forward(std::shared_ptr<Channel> channel,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;
    // get or create output
    std::shared_ptr<IClientConn> forward_conn;
    std::shared_ptr<Pool<OutputHandler>> output_pool = server->get_output_pool();
    std::shared_ptr<OutputHandler> output = std::make_shared<OutputHandler>(output_pool, channel);
    output->init_conn(forward_conn);

    // create mux based on request
    std::string output_ext = "";
    std::shared_ptr<IMux> muxer = create_mux_by_ext(output_ext);
    /*if (req->format == "RAW") {  // to do
        muxer = std::make_shared<RawMux>();
    } else {
        muxer = std::make_shared<RawMux>();
    }   //*/
    output->init_format(muxer);

    std::string forward_host = req->vhost;
    int forward_port = 80;
    std::string forward_url = create_channel_key(req);
    Address address(forward_host, forward_port);
    output->set_forward_address(address);
    output->set_forward_url(forward_url);

    std::shared_ptr<IContext> context = create_context_by_ext(output_ext);
    output->set_context(context);

    output->set_type(EOutputForawrd);

    channel->add_output(output);

    output->run();

    return ret;
}

int MediaSource::create_origin_file(std::shared_ptr<File>& file,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;

    std::string key =
        req->vhost + req->path + req->name;
    file = std::make_shared<File>(key);
    server->get_file_cache()->add_file(key, file);

    // get or create input
    // connect to origin

    std::string origin_host = req->vhost;   //  to do
    std::string origin_path = req->path;
    std::string stream = req->name;
    std::string param = req->params;
    std::shared_ptr<IClientConn> origin_conn = std::make_shared<TcpStreamConn>(nullptr);

    std::shared_ptr<FileInputHandler> input =
        std::make_shared<FileInputHandler>(file, server->get_file_input_pool());

    std::shared_ptr<IContext> context;
    std::shared_ptr<IDeMux> demuxer;
    demuxer = std::make_shared<RawDeMux>();
    context = std::make_shared<IContext>();

    input->set_context(context);
    input->set_type(EInputOrigin);
    input->init_format(demuxer);
    input->init_conn(origin_conn);
    input->init_input();

    std::string origin_ip = origin_host;     //  to do
    int origin_port = 80;
    Address address(origin_ip, origin_port);
    //  input->set_origin_address(address);
    input->set_origin_info(address, origin_host, origin_path, stream, "", param);

    // file->add_input(input);
    input->run();

    return ret;
}

int file_output_func(void *opaque, uint8_t *buf, int buf_size) {
    IClientConn* conn = static_cast<IClientConn*>(opaque);
    return conn->write(reinterpret_cast<char*>(buf), buf_size);
}

int MediaSource::handle_play_file(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;
    // get or create Channel
    std::string key =
        req->vhost + req->path + req->name;

    std::shared_ptr<FileCache> file_cache = server->get_file_cache();
    std::shared_ptr<File> file = file_cache->get_file(key);
    int64_t current_time_ms = get_cache_time() / 1000;

    if (file &&
        (((current_time_ms - file->get_update_time()) < 1000) || !file->complete())) {
        // get the file, when the file is complete and not outdate
        tmss_info("get the file");
    } else {
        // check if it is need origin
        tmss_info("create the file");
        create_origin_file(file, req, server);
    }

    std::shared_ptr<IMux> muxer = std::make_shared<RawMux>();
    int out_buf_size = 1024 * 16;
    uint8_t* out_buf = new uint8_t[out_buf_size];
    std::shared_ptr<IContext> context = std::make_shared<IContext>();
    muxer->init_output(out_buf, out_buf_size,
        conn.get(), file_output_func, static_cast<void*>(context.get()), static_cast<void*>(context.get()));

    int offset = 0;
    while (!(file->complete()) || (offset < file->get_total_length())) {
        char buffer[1024];
        int size = sizeof(buffer);
        ret = file->seek_range(buffer, size, offset);     // timeout
        if (ret != error_success) {
            tmss_error("file read error,{},offset={}", ret, offset);
            break;
        }
        offset += size;
        ret = muxer->send_status(200);
        if (ret != error_success) {
            tmss_error("send http header error,{}", ret);
            break;
        }

        std::shared_ptr<SimplePacket> raw_packet =
            std::make_shared<SimplePacket>(buffer, size);
        ret = muxer->handle_output(raw_packet);
        if (ret < 0) {
            tmss_error("file send error,{}", ret);
            break;
        }
        tmss_info("size={},offset={},file_length={},file_complete={}",
            size, offset, file->get_total_length(), file->complete());
    }

    tmss_info("file send complete");

    conn->set_stop();

    return ret;
}

int MediaSource::handle_play_segment(std::shared_ptr<IClientConn> conn,
        std::shared_ptr<Request> req,
        std::shared_ptr<IServer> server) {
    int ret = error_success;
    // get or create Channel
    std::string stream_key = create_channel_key(req);
    tmss_info("handle_play_stream,stream_key={}", stream_key.c_str());
    std::shared_ptr<Channel> channel;
    std::shared_ptr<ChannelMgr> channel_mgr = server->get_channel_mgr();
    channel_mgr->fetch_or_create_channel(stream_key, channel);
    channel->init_user_ctrl(shared_from_this());

    std::shared_ptr<SegmentsCache> segment_cache = channel->segment_cache;
    if (!segment_cache) {
        segment_cache = std::make_shared<SegmentsCache>(3, 10);
        channel->add_segemt_cache(segment_cache);

        // segment
        segment_cache->set_format(req->format);
        //  segment_cache->set_output_context(context);
        segment_cache->init();
        channel->add_segemt_cache(segment_cache);
        tmss_info("handle_play_segment");
    }
    if (channel->get_status() == EChannelStart) {
        tmss_info("channel is already running");
    } else {
        // check if it need origin
        create_origin_stream(channel, req, server);

        // start channel
        ret = channel->run();
        if (ret != error_success) {
            tmss_error("channel run failed,ret={}", ret);
            channel->on_stop();
            //  return ret;
        }
    }

    tmss_info("handle_play_segment");

    std::string key =
        req->vhost + req->path + req->name;
    std::shared_ptr<IMux> muxer = std::make_shared<RawMux>();
    int out_buf_size = 1024 * 16;
    uint8_t* out_buf = new uint8_t[out_buf_size];
    std::shared_ptr<IContext> context = std::make_shared<IContext>();
    muxer->init_output(out_buf, out_buf_size,
        conn.get(), file_output_func, static_cast<void*>(context.get()), static_cast<void*>(context.get()));

    std::shared_ptr<FileCache> file_cache = segment_cache->get_file_cache();
    std::shared_ptr<File> file = file_cache->get_file(key);

    int64_t current_time_ms = get_cache_time() / 1000;
    if (file &&
        (((current_time_ms - file->get_update_time()) < 1000 * 1000) || !file->complete())) {
        // get the file, when the file is complete and not outdate
        tmss_info("get the file");
    } else {
        // check if it is need origin
        tmss_info("not find the file or expire");
        ret = muxer->send_status(404);
        if (ret != error_success) {
            tmss_error("send http header error,{}", ret);
            return ret;
        }
        tmss_info("send http 404");
        conn->close();
        return ret;
    }

    int offset = 0;
    while (!(file->complete()) || (offset < file->get_total_length())) {
        char buffer[1024];
        int size = sizeof(buffer);
        ret = file->seek_range(buffer, size, offset);     // timeout
        if (ret != error_success) {
            tmss_error("file read error,{},offset={}", ret, offset);
            break;
        }
        offset += size;
        ret = muxer->send_status(200);
        if (ret != error_success) {
            tmss_error("send http header error,{}", ret);
            break;
        }

        std::shared_ptr<SimplePacket> raw_packet =
            std::make_shared<SimplePacket>(buffer, size);
        ret = muxer->handle_output(raw_packet);
        if (ret < 0) {
            tmss_error("file send error,{}", ret);
            break;
        }
        tmss_info("size={},offset={},file_length={},file_complete={}",
            size, offset, file->get_total_length(), file->complete());
    }

    tmss_info("file send complete");

    conn->set_stop();

    return ret;
}

int init_default_handler(int num, char** param) {
    int level = 0;
    std::shared_ptr<MediaSource> media_source = std::make_shared<MediaSource>();
    HttpMux::get_instance()->register_handler("/", level++, media_source);
    int ret = media_source->init(num, param);
    if (ret != error_success) {
        tmss_error("media_souce init error, ret={}", ret);
        return ret;
    }
    return ret;
}

}  // namespace tmss

