/*
 * TMSS
 * Copyright (c) 2021 rainwu
 */
#pragma once

namespace tmss {
class IFrame {
 public:
    virtual ~IFrame() = default;

 public:
    virtual char*  buffer() = 0;
    virtual int     get_size()  = 0;
    virtual int64_t timestamp() = 0;
};
}  // namespace tmss
