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

#include <memory>
#include <protocol/client.hpp>
#include <rtmp_stack.hpp>
#include <net/tmss_conn.hpp>
#include <coroutine/coroutine.hpp>
#include <cache/tmss_channel.hpp>
#include <tmss_user_control.hpp>
#include <parser.hpp>

namespace tmss {
class IDeMux;
class RtmpClient : public IClient {
 public:
    RtmpClient();
    virtual ~RtmpClient() = default;

 public:
    virtual int cycle() override;
    virtual int request(const std::string& origin_host,
        const std::string& origin_path,
        const std::string& stream,
        const std::string& param,
        std::shared_ptr<IDeMux> demux);
    virtual int read_data(char* buf, int size) override;

    int write_data(const char* buf, int size) override;

 private:
    virtual int connect(const std::string &origin_host,
            const std::string &request_url,
            int &stream_id,
            const std::string &path,
            std::shared_ptr<IClientConn> conn);
    /**
     * set req to use the original request of client:
     *      pageUrl and swfUrl for refer antisuck.
     *      args for edge to origin traverse auth, @see SrsRequest.args
     */
    virtual int connect_app(std::string path, std::string tc_url);
    /**
     * handshake with server, try complex, then simple handshake.
     */
    virtual int handshake(std::shared_ptr<IClientConn> conn);
    /**
     * only use simple handshake
     */
    virtual int simple_handshake(std::shared_ptr<IClientConn> conn);
    /**
     * only use complex handshake
     */
    virtual int complex_handshake(std::shared_ptr<IClientConn> conn);

    /**
     * create a stream, then play/publish data over this stream.
     */
    virtual int create_stream(int& stream_id);
    /**
     * start play stream.
     */
    virtual int play(const std::string& stream, int stream_id);
    /**
     * start publish stream. use flash publish workflow:
     *       connect-app => create-stream => flash-publish
     */
    virtual int publish(std::string stream, int stream_id);
    /**
     * start publish stream. use FMLE publish workflow:
     *       connect-app => FMLE publish
     */
    virtual int fmle_publish(std::string stream, int& stream_id);

 private:
     std::shared_ptr<RtmpProtocolHandler> protocol;
};

}  // namespace tmss
