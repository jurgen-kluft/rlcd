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

namespace ncore
{
    struct state_app_t
    {
        xor_random_t gRandom;  // XORShift random number generator for any randomization needs
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

        ntimer::periodic_task_t gBlinkLedTask = {1000, 0, []() { nlcd::nwcs::led_toggle(); }};

        void setup(state_t* state)
        {
            if (nlcd::nwcs::initialize() == false)
            {
                nlog::println("Failed to initialize LCD");
            }

            nlog::println("Setup complete");
        }

        static u64 toggle_lcd_fill_time = 0;

        void tick(state_t* state)
        {
            ntimer::tick(&gBlinkLedTask);

            if (ntimer::millis() - toggle_lcd_fill_time > 2000)
            {
                nlog::println("Filling random area with random color");

                // Fill a random area with a random color every 2 seconds
                u16 color = (u16)g_random_u32(&gAppState.gRandom, 16);  // Random RGB565 color
                u16 sx    = (u16)g_random_u32_range(&gAppState.gRandom, 0, nlcd::nwcs::width() - 51);
                u16 sy    = (u16)g_random_u32_range(&gAppState.gRandom, 0, nlcd::nwcs::height() - 51);
                u16 ex    = sx + (g_random_u32_range(&gAppState.gRandom, 0, 40) + 10);  // Random width between 10 and 50 pixels
                u16 ey    = sy + (g_random_u32_range(&gAppState.gRandom, 0, 40) + 10);  // Random height between 10 and 50 pixels

                nlcd::nwcs::draw_rectangle(sx, sy, ex, ey, color);
                toggle_lcd_fill_time = ntimer::millis();
            }
        }

    }  // namespace napp
}  // namespace ncore