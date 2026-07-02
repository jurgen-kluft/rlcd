#include <Arduino.h>

#include "ccore/c_memory.h"
#include "ccore/c_va_list.h"
#include "rcore/c_log.h"

#include "lib_guition/c_lcd.h"

#define GUITION_LCD_H_RES 480
#define GUITION_LCD_V_RES 480

namespace ncore
{

    static bool s_guition_display_init() { return true; }

    static bool s_guition_display_brightness_init() { return true; }

    static void s_guition_display_backlight_on() {}
    static void s_guition_display_backlight_off() {}

    static bool s_guition_touch_init() { return true; }

    namespace nlcd
    {
        static i8 g_led_brightness = 100;

        bool initialize()
        {
            ncore::nlog::log_info("LCD", "Initializing LCD");

            if (!s_guition_display_init())
            {
                ncore::nlog::log_error("LCD", "Failed to initialize LCD display");
                return false;
            }

            if (s_guition_display_brightness_init() != ESP_OK)
            {
                ncore::nlog::log_error("LCD", "Failed to initialize LCD display brightness");
                return false;
            }

            if (s_guition_touch_init() != ESP_OK)
            {
                ncore::nlog::log_error("LCD", "Failed to initialize touch panel");
                return false;
            }

            // ledcAttach(2, 600, 8);

            return true;
        }

        u16 width() { return GUITION_LCD_H_RES; }
        u16 height() { return GUITION_LCD_V_RES; }

        void draw_rectangle(u16 sx, u16 sy, u16 ex, u16 ey, u16 color) {}
        void draw_sprite(u16 sx, u16 sy, u16 ex, u16 ey, const u16 *color) {}

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
                s_guition_display_backlight_on();
            }
            else
            {
                s_guition_display_backlight_off();
            }
        }

        // display can be turned on/off
        void display_power(bool on)
        {
            if (on)
            {
                s_guition_display_backlight_on();
            }
            else
            {
                s_guition_display_backlight_off();
            }
        }

    }  // namespace nlcd
}  // namespace ncore
