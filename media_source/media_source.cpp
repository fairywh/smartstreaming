/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */


#include "media_source_handler.hpp"

namespace tmss {
const int MaxHostNameLen = 100;
extern "C" void* init_user_control(int num, char ** param) {
    int level = 0;
    std::shared_ptr<MediaSource> media_source = std::make_shared<MediaSource>();
    HttpMux::get_instance()->register_handler("/", level++, media_source);
    media_source->init(num, param);
    return nullptr;
}
}

