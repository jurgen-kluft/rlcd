#include "rcore/c_app.h"
#include "rcore/c_gpio.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_packet.h"
#include "rcore/c_str.h"
#include "rcore/c_system.h"
#include "rcore/c_wire.h"
#include "ccore/c_random.h"

#include "lib_guition/c_lcd.h"
#include "lib_guition/c_sdcard.h"
#include "lib_touch/c_touch_gt911.h"

namespace ncore
{
    namespace napp
    {
        static ntouch::touch_t gTouchPanel;
        static xor_random_t    gRandom;

        void wakeup(state_t *state, ncore::nwakeup::reason_t reason)
        {
            // Handle wakeup reasons if needed (e.g., from deep sleep)
        }

        // ntimer::periodic_task_t gBlinkLedTask;

        void setup(state_t *state)
        {
            // Time critical setup before WiFi and other components are initialized can be done here

            // ntimer::init_periodic_task(&gBlinkLedTask, 1000, nullptr, [](void* user) { nlcd::led_toggle(); });
            nlog::log_info("main", "setup");

            if (nlcd::initialize() == false)
            {
                nlog::log_error("main", "Failed to initialize LCD");
            }
            else
            {
                nlog::log_info("main", "LCD initialized successfully");

                const u8  i2c_addr = 0x5D;  // GT911 I2C address
                const i8  sda_pin  = 19;    // GT911 SDA pin
                const i8  scl_pin  = 45;    // GT911 SCL pin
                const i8  int_pin  = -1;    // GT911 INT pin (not used)
                const i8  rst_pin  = -1;    // GT911 RST pin (not used)
                const u16 width    = nlcd::width();
                const u16 height   = nlcd::height();

                if (ntouch::ngt911::touch_init(gTouchPanel, width, height, 50, i2c_addr, sda_pin, scl_pin, int_pin, rst_pin, ntouch::cTP_ROTATE_0, ntouch::cTP_MIRROR_NONE) == false)
                {
                    nlog::log_error("main", "Failed to initialize touch panel");
                }
                else
                {
                    nlog::log_info("main", "Touch panel initialized successfully");
                }

                // if (nlcd::sdcard_initialize())
                // {
                //     u64 total_bytes, free_bytes;
                //     if (nlcd::sdcard_get_usage(&total_bytes, &free_bytes))
                //     {
                //         nlog::printfln("SD card total size: %.2f MB", va_t(total_bytes / (1024.0 * 1024.0)));
                //         nlog::printfln("SD card free space: %.2f MB", va_t(free_bytes / (1024.0 * 1024.0)));
                //     }
                // }
            }
        }

        static u64 toggle_lcd_fill_time = 0;

        void tick(state_t *state)
        {
            const u64 now_ms = ntimer::millis();

            // if (touch_touched(now_ms))
            // {
            //     nlog::log_infof("main", "Touch detected at: (%d, %d)", va_list_t(va_t(touch_last_x), va_t(touch_last_y)));
            //     toggle_lcd_fill_time = now_ms;  // Reset the fill timer when a touch is detected
            // }

            u8                    num_points = 0;
            ntouch::touch_point_t points[ntouch::TP_CT_MAX_TOUCH];
            if (ntouch::touch_scan(gTouchPanel, now_ms, points, ntouch::TP_CT_MAX_TOUCH, &num_points) == true)
            {
                nlog::log_infof("main", "Touch detected: %u points", va_list_t(va_t(num_points)));
                for (u8 i = 0; i < num_points; i++)
                {
                    const ntouch::touch_point_t *point = &points[i];
                    if (point != nullptr)
                    {
                        nlog::log_infof("main", "Point %u: (%u, %u, %u)", va_list_t(va_t(i + 1), va_t(point->m_x), va_t(point->m_y), va_t(point->m_size)));

                        // nlcd::draw_rectangle(point->m_x, point->m_y, point->m_x + 1, point->m_y + 1, 0xF800);  // Draw a red rectangle around the touch point
                    }
                }
                toggle_lcd_fill_time = now_ms;  // Reset the fill timer when a touch is detected
            }

            if (now_ms - toggle_lcd_fill_time > 2000)
            {
                nlog::log_info("main", "Filling random area with random color");

                // Fill a random area with a random color every 2 seconds
                u16 color = (u16)g_random_u32(&gRandom, 16);  // Random RGB565 color
                u16 sx    = (u16)g_random_u32_range(&gRandom, 0, nlcd::width() - 51);
                u16 sy    = (u16)g_random_u32_range(&gRandom, 0, nlcd::height() - 51);
                u16 ex    = sx + (g_random_u32_range(&gRandom, 0, 40) + 10);  // Random width between 10 and 50 pixels
                u16 ey    = sy + (g_random_u32_range(&gRandom, 0, 40) + 10);  // Random height between 10 and 50 pixels

                // nlcd::draw_rectangle(sx, sy, ex, ey, color);
                toggle_lcd_fill_time = now_ms;
            }
        }

    }  // namespace napp
}  // namespace ncore