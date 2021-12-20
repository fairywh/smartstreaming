/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng.
 *
 * =====================================================================================
 */


#include <format/demux.hpp>

namespace tmss {
std::shared_ptr<IContext> IDeMux::get_context() {
    return ctx;
}
void IDeMux::set_context(std::shared_ptr<IContext> context) {
    ctx = context;
}

int IDeMux::read_packet(char* buf, int size) {
    return read_packet_func(opaque,
        reinterpret_cast<uint8_t*>(buf),
        size);
}
}   // namespace tmss