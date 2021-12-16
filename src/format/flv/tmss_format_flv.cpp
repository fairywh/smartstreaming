/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include <flv/tmss_format_flv.hpp>
#include <log/log.hpp>
namespace tmss {
const int max_bufer_size = 1 * 1024 * 1024;
FlvDeMux::FlvDeMux() {
    this->set_format("flv");
}

FlvDeMux::~FlvDeMux() {
}

FlvMux::FlvMux() {
    this->set_format("flv");
}

FlvMux::~FlvMux() {
}

}  // namespace tmss

