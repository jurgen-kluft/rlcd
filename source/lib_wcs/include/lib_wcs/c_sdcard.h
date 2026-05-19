#ifndef __RLCD_WCS_SDCARD_H__
#define __RLCD_WCS_SDCARD_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nlcd
    {
        bool sdcard_initialize();

        bool sdcard_get_usage(u64 *out_total_bytes, u64 *out_free_bytes);
    }  // namespace nlcd
}  // namespace ncore

#endif  // __RLCD_WCS_SDCARD_H__
