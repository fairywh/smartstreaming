/* Copyright [2020] <Tencent, China>
 *
 * =====================================================================================
 *        Version:  1.0
 *        Created:  on 2021/3/25.
 *        Author:  weideng.
 *
 * =====================================================================================
 */

#pragma once

namespace tmss {
class IPacket {
 public:
    virtual ~IPacket() = default;

 public:
    virtual char*  buffer() = 0;
    virtual int     get_size()  = 0;
    virtual int64_t timestamp() = 0;
    virtual bool is_key_frame() = 0;
};
}  // namespace tmss
