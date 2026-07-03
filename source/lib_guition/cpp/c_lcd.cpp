#include <Arduino.h>

#include "ccore/c_memory.h"
#include "ccore/c_va_list.h"
#include "rcore/c_log.h"

#include "lib_guition/c_lcd.h"
#include "lib_display/display_st7701.h"
#include "lib_touch/c_touch_gt911.h"

#define GUITION_LCD_H_RES 480
#define GUITION_LCD_V_RES 480

namespace ncore
{

#define GFX_BL 38

    static i8   g_led_brightness = 100;
    static void s_guition_display_backlight_set_brightness(i8 brightness)
    {
        g_led_brightness = brightness;
        const i32 pwmp   = ((i32)brightness * 255) / 100;  // Convert percentage to PWM value (0-255)
        ledcWrite(GFX_BL, pwmp);
    }

    static void s_guition_display_brightness_full()
    {
        s_guition_display_backlight_set_brightness(100);  // Set brightness to 100% when turning on
    }

    static void s_guition_display_brightness_off()
    {
        s_guition_display_backlight_set_brightness(0);  // Set brightness to 0% when turning off
    }

    static bool s_guition_display_brightness_init()
    {
        // Pin routing, frequency (600Hz), and resolution (8-bit) are now set in one step.
        // Channels are automatically assigned, removing the need for a channel ID (like '0').
        ledcAttach(GFX_BL, 600, 8);

        s_guition_display_brightness_full();
        return true;
    }

    static Arduino_DataBus*       g_display_databus  = nullptr;
    static Arduino_ESP32RGBPanel* g_display_rgbpanel = nullptr;
    static Arduino_RGB_Display*   g_display_gfx      = nullptr;

    static bool s_guition_display_init()
    {
        initialize_st7701_display(g_display_databus, g_display_rgbpanel, g_display_gfx);

        if (s_guition_display_brightness_init() == false)
        {
            nlog::log_error("LCD", "Failed to initialize display brightness");
            return false;
        }

        return true;
    }

    namespace nlcd
    {
        bool initialize()
        {
            ncore::nlog::log_info("LCD", "Initializing LCD");

            if (s_guition_display_init() == false)
            {
                ncore::nlog::log_error("LCD", "Failed to initialize LCD display");
                return false;
            }

            if (s_guition_display_brightness_init() == false)
            {
                ncore::nlog::log_error("LCD", "Failed to initialize LCD display brightness");
                return false;
            }

            // ledcAttach(2, 600, 8);
            return g_display_gfx->begin();
        }

        u16 width() { return GUITION_LCD_H_RES; }
        u16 height() { return GUITION_LCD_V_RES; }

        void draw_rectangle(u16 sx, u16 sy, u16 ex, u16 ey, u16 color) 
        {
            g_display_gfx->fillRect(sx, sy, ex - sx + 1, ey - sy + 1, color);

        }
        void draw_sprite(u16 sx, u16 sy, u16 ex, u16 ey, const u16* color) {}

        void led_toggle()
        {
            g_led_brightness = (g_led_brightness == 0) ? 100 : 0;
            // ledcWrite(0, g_led_brightness);
        }

        void led_switch(bool on)
        {
            g_led_brightness = on ? 100 : 0;
            // ledcWrite(0, g_led_brightness);
        }

        // display has a backlight control pin, which can be used to turn on/off the backlight
        void backlight_switch(bool on)
        {
            if (on)
            {
                s_guition_display_brightness_full();
            }
            else
            {
                s_guition_display_brightness_off();
            }
        }

        // display can be turned on/off
        void display_power(bool on)
        {
            if (on)
            {
                s_guition_display_brightness_full();
            }
            else
            {
                s_guition_display_brightness_off();
            }
        }

    }  // namespace nlcd
}  // namespace ncore
