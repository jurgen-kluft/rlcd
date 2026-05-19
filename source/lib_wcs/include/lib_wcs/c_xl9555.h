#ifndef __RLCD_WCS_XL9555_H__
#define __RLCD_WCS_XL9555_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nxl9555
    {
        enum io_pin_t
        {
            TP_RST_IO  = 0x0001,
            LCD_RST_IO = 0x0002,
            SD_CS_IO   = 0x0004,
            BL_CTR_IO  = 0x0008,
            LED1_IO    = 0x0010,
            IO5        = 0x0020,
            IO6        = 0x0040,
            IO7        = 0x0080,
            IO8        = 0x0100,
            IO9        = 0x0200,
            IO10       = 0x0400,
            IO11       = 0x0800,
            IO12       = 0x1000,
            IO13       = 0x2000,
            IO14       = 0x4000,
            IO15       = 0x8000,
        };

        bool init();
        i16  pin_read(u16 pin);
        u16  pin_write(u16 pin, i32 val);
        bool read_byte(u8 reg, u8 *data, u32 len);
        bool write_byte(u8 reg, u8 *data, u32 len);
        bool pin_toggle(u16 pin);
    }  // namespace nxl9555
}  // namespace ncore

#endif  // __RLCD_WCS_XL9555_H__
