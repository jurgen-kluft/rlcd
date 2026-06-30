#include "rcore/c_gpio.h"

#include "lib_guition/c_relay.h"

namespace ncore
{
    namespace nrelay
    {
        static ngpio::output_pin_t relay_pins[] = {
          ngpio::output_pin_t(40),  // RELAY_1
          ngpio::output_pin_t(1),   // RELAY_2
          ngpio::output_pin_t(2),   // RELAY_3
        };

        void relay_init()
        {
            for (i32 i = 0; i < DARRAYSIZE(relay_pins); ++i)
            {
                relay_pins[i].setup();
                relay_pins[i].set_low();
            }
        }

        void relay_on(erelay_t relay_id)
        {
            if (relay_id >= RELAY_1 && relay_id <= RELAY_3)
            {
                relay_pins[relay_id].set_high();
            }
        }

        void relay_off(erelay_t relay_id)
        {
            if (relay_id >= RELAY_1 && relay_id <= RELAY_3)
            {
                relay_pins[relay_id].set_low();
            }
        }

    }  // namespace nrelay
}  // namespace ncore
