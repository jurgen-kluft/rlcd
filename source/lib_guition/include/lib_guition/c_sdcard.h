#ifndef __RLCD_GUITION_SDCARD_H__
#define __RLCD_GUITION_SDCARD_H__
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
        bool sdcard_read_file(const char* path, u8* buffer, u32 buffer_size, u32& out_buffer_size);
        bool sdcard_read_bytes(const char* path, u32 offset, u8* buffer, u32 size_to_read);
        bool sdcard_write_file(const char* path, const u8* buffer, u32 buffer_size);
    }  // namespace nlcd
}  // namespace ncore

#endif  // __RLCD_GUITION_SDCARD_H__
