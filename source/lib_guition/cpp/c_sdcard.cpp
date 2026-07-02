#include <Arduino.h>

#include "SPI.h"
#include "SD.h"
#include "FS.h"

#include "rcore/c_log.h"
#include "rcore/c_gpio.h"
#include "ccore/c_memory.h"

#include "lib_guition/c_sdcard.h"

// -------------------------------------------------------------------------------------------------
// ------ SD Card SPI ------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
namespace ncore
{
    namespace nlcd
    {

#define SD_CS    42
#define SPI_MOSI 47
#define SPI_MISO 41
#define SPI_SCK  48

        bool sdcard_initialize()
        {
            ngpio::output_pin_t cs_pin(SD_CS);
            cs_pin.setup();
            cs_pin.set_high();

            SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

            if (!SD.begin(SD_CS, SPI, 1000000))
            {
                nlog::println("SDCard: Failed to mount SD card");
                return false;
            }
            uint8_t cardType = SD.cardType();
            if (cardType == CARD_NONE)
            {
                nlog::println("SDCard: No SD card attached");
                return false;
            }
            nlog::println("SDCard: SD Card initialized.");
            return true;
        }

        bool sdcard_get_usage(u64 *out_total_bytes, u64 *out_free_bytes)
        {
            uint8_t cardType = SD.cardType();
            if (cardType == CARD_NONE)
            {
                nlog::println("SDCard: No SD card attached");
                return false;
            }

            u64 total_bytes = SD.totalBytes();
            u64 free_bytes  = SD.usedBytes();

            if (out_total_bytes != nullptr)
                *out_total_bytes = total_bytes;
            if (out_free_bytes != nullptr)
                *out_free_bytes = free_bytes;

            return true;
        }

    }  // namespace nlcd
}  // namespace ncore