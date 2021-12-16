/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng(邓伟).
 *
 * =====================================================================================
 */


#include <format/mux.hpp>

namespace tmss {
std::shared_ptr<IContext> IMux::get_context() {
    return ctx;
}
void IMux::set_context(std::shared_ptr<IContext> context) {
    ctx = context;
}

}   // namespace tmss