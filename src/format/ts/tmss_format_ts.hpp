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
class MpegTsDeMux : virtual public BaseDeMux {
 public:
    MpegTsDeMux();
    ~MpegTsDeMux();
};

/*
*   data flow with long connection
*/
class MpegTsMux : virtual public BaseMux {
 public:
    MpegTsMux();
    ~MpegTsMux();
};
}  // namespace tmss

