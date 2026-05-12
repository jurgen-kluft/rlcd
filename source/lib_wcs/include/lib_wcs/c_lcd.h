#ifndef __RLCD_WCS_LCD_H__
#define __RLCD_WCS_LCD_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nlcd
    {
        namespace nwcs
        {
            bool initialize();

            void rotation(uint8_t rotation);
            void orientation(uint8_t orientation);

            void draw_rectangle(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);
            void draw_sprite(u16 sx, u16 sy, u16 ex, u16 ey, const u16 *color);

            // WCS display is a full ESP32-S3 board and has an RGB LED on board
            void led_toggle();
            void led_switch(bool on);

            // WCS display has a backlight control pin, which can be used to turn on/off the backlight
            void backlight_switch(bool on);

        }  // namespace nwcs
    }  // namespace nlcd
}  // namespace ncore

#endif  // __RLCD_WCS_LCD_H__
