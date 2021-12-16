/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include <ts/tmss_format_ts.hpp>
#include <log/log.hpp>
namespace tmss {
const int max_bufer_size = 1 * 1024 * 1024;
MpegTsDeMux::MpegTsDeMux() {
    this->set_format("mpegts");
}

MpegTsDeMux::~MpegTsDeMux() {
}

MpegTsMux::MpegTsMux() {
    this->set_format("mpegts");
}

MpegTsMux::~MpegTsMux() {
}

}  // namespace tmss

