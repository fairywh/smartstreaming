/* Copyright [2021] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021.
 *        Author:  rainwu
 *
 * =====================================================================================
 */

#pragma once
#include <map>
#include <string>
#include <format/base/tmss_format_base.hpp>


namespace tmss {
class FlvDeMux : virtual public BaseDeMux {
 public:
    FlvDeMux();
    ~FlvDeMux();
};

/*
*   data flow with long connection
*/
class FlvMux : virtual public BaseMux {
 public:
    FlvMux();
    ~FlvMux();
};
}  // namespace tmss

