#ifndef __RLCD_LIBTOUCH_TOUCH_GT911_H__
#define __RLCD_LIBTOUCH_TOUCH_GT911_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "lib_touch/c_touch.h"

namespace ncore
{
    namespace ntouch
    {
        namespace ngt911
        {
            bool touch_init(touch_t& tp, u16 width, u16 height, u64 polling_interval_ms, u8 i2c_addr, i8 sda_pin, i8 scl_pin, i8 int_pin, i8 rst_pin, erotate_t rotation, emirror_t mirror);
        }  // namespace ngt911
    }  // namespace ntouch
}  // namespace ncore

#endif  // __RLCD_LIBTOUCH_TOUCH_GT911_H__