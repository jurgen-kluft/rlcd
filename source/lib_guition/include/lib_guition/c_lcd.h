#ifndef __RLCD_GUITION_LCD_H__
#define __RLCD_GUITION_LCD_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nlcd
    {
        bool initialize();
        u16  width();
        u16  height();

        void draw_rectangle(u16 sx, u16 sy, u16 ex, u16 ey, u16 color);
        void draw_sprite(u16 sx, u16 sy, u16 ex, u16 ey, const u16 *color);

        void led_toggle();
        void led_switch(bool on);

        // display has a backlight control pin, which can be used to turn on/off the backlight
        void backlight_switch(bool on);

        // display can be turned on/off
        void display_power(bool on);

    }  // namespace nlcd
}  // namespace ncore

#endif  // __RLCD_GUITION_LCD_H__
