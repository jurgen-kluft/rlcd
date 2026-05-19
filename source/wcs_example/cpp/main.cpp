#include "lib_wcs/dummy.h"

#include "rcore/c_app.h"
#include "rcore/c_gpio.h"
#include "rcore/c_timer.h"
#include "rcore/c_log.h"
#include "rcore/c_packet.h"
#include "rcore/c_str.h"
#include "rcore/c_system.h"
#include "rcore/c_task.h"
#include "rcore/c_wire.h"
#include "ccore/c_random.h"

#include "lib_wcs/c_lcd.h"
#include "lib_wcs/c_touch.h"
#include "lib_wcs/c_sdcard.h"

namespace ncore
{
    struct state_app_t
    {
        xor_random_t          gRandom;      // XORShift random number generator for any randomization needs
        ntouch::touch_panel_t gTouchPanel;  // Touch panel state
    };
    state_app_t  gAppState;
    state_task_t gAppTask;

}  // namespace ncore

namespace ncore
{
    namespace napp
    {
        void wakeup(state_t* state, ncore::nwakeup::reason_t reason)
        {
            // Handle wakeup reasons if needed (e.g., from deep sleep)
        }

        void presetup(state_t* state)
        {
            // Time critical setup before WiFi and other components are initialized can be done here
        }

        ntimer::periodic_task_t gBlinkLedTask = {1000, 0, []() { nlcd::led_toggle(); }};

        void setup(state_t* state)
        {
            nlog::println("Setup begin");
            
            if (nlcd::initialize() == false)
            {
                nlog::println("Failed to initialize LCD");
            }
            else
            {
                if (ntouch::tp_init(gAppState.gTouchPanel, nlcd::width(), nlcd::height()) == false)
                {
                    nlog::println("Failed to initialize touch panel");
                }

                if (nlcd::sdcard_initialize() == false)
                {
                    nlog::println("Failed to initialize SD card");
                }
                else
                {
                    nlog::println("SD card initialized successfully");

                    u64 total_bytes, free_bytes;
                    if (nlcd::sdcard_get_usage(&total_bytes, &free_bytes))
                    {
                        nlog::printfln("SD card total size: %.2f MB", va_t(total_bytes / (1024.0 * 1024.0)));
                        nlog::printfln("SD card free space: %.2f MB", va_t(free_bytes / (1024.0 * 1024.0)));
                    }
                }

                nlog::println("Setup complete");
            }
        }

        static u64 toggle_lcd_fill_time = 0;

        void tick(state_t* state)
        {
            ntimer::tick(&gBlinkLedTask);

            if (ntouch::tp_scan(gAppState.gTouchPanel, 0))
            {
                u8 num_points = ntouch::tp_get_touch_point_num(gAppState.gTouchPanel);
                nlog::printfln("Touch detected with %d point(s)", va_t(num_points));
                for (u8 i = 0; i < num_points; i++)
                {
                    if (ntouch::tp_is_valid_touch_point(gAppState.gTouchPanel, i))
                    {
                        ntouch::touch_point_t point = ntouch::tp_get_touch_point(gAppState.gTouchPanel, i);
                        // nlog::printfln("  Point %d: (%d, %d)", va_t(i + 1), va_t(point.x), va_t(point.y));

                        nlcd::draw_rectangle(point.x, point.y, point.x + 1, point.y + 1, 0xF800);  // Draw a red rectangle around the touch point
                        toggle_lcd_fill_time = ntimer::millis();
                    }
                }
            }

            if (ntimer::millis() - toggle_lcd_fill_time > 2000)
            {
                nlog::println("Filling random area with random color");

                // Fill a random area with a random color every 2 seconds
                u16 color = (u16)g_random_u32(&gAppState.gRandom, 16);  // Random RGB565 color
                u16 sx    = (u16)g_random_u32_range(&gAppState.gRandom, 0, nlcd::width() - 51);
                u16 sy    = (u16)g_random_u32_range(&gAppState.gRandom, 0, nlcd::height() - 51);
                u16 ex    = sx + (g_random_u32_range(&gAppState.gRandom, 0, 40) + 10);  // Random width between 10 and 50 pixels
                u16 ey    = sy + (g_random_u32_range(&gAppState.gRandom, 0, 40) + 10);  // Random height between 10 and 50 pixels

                nlcd::draw_rectangle(sx, sy, ex, ey, color);
                toggle_lcd_fill_time = ntimer::millis();
            }
        }

    }  // namespace napp
}  // namespace ncore