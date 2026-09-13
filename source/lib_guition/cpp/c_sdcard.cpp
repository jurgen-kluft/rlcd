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
                nlog::log_error("SDCard", "Failed to mount SD card");
                return false;
            }
            uint8_t cardType = SD.cardType();
            if (cardType == CARD_NONE)
            {
                nlog::log_error("SDCard", "No SD card attached");
                return false;
            }
            nlog::log_info("SDCard", "SD Card initialized.");
            return true;
        }

        bool sdcard_get_usage(u64* out_total_bytes, u64* out_free_bytes)
        {
            uint8_t cardType = SD.cardType();
            if (cardType == CARD_NONE)
            {
                nlog::log_error("SDCard", "No SD card attached");
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

        bool sdcard_read_file(const char* path, u8* buffer, u32 buffer_capacity, u32& out_buffer_size)
        {
            SDFile file = SD.open(path);
            if (!file)
            {
                return false;
            }

            const u32 file_size = file.size();
            if (file_size > buffer_capacity)
            {
                file.close();
                return false;
            }

            const u32 bytes_read = file.read(buffer, file_size);
            file.close();

            out_buffer_size = bytes_read;
            return bytes_read == file_size;
        }

        bool sdcard_read_bytes(const char* path, u32 offset, u8* buffer, u32 size_to_read)
        {
            SDFile file = SD.open(path);
            if (!file)
            {
                return false;
            }

            const u32 file_size = file.size();
            if (offset + size_to_read > file_size)
            {
                file.close();
                return false;
            }

            file.seek(offset);
            const u32 bytes_read = file.read(buffer, size_to_read);
            file.close();

            return bytes_read == size_to_read;
        }

        bool sdcard_write_file(const char* path, const u8* buffer, u32 buffer_size)
        {
            // 1. Open the file in write mode ("w").
            // This creates the file if it doesn't exist, or truncates (clears) it if it does.
            File file = SD.open(path, FILE_WRITE);
            if (!file)
            {
                nlog::log_warnf("SDCard", "failed to open file for writing: %s\n", va_list_t(va_t(path)));
                return false;
            }

            // 2. Write the buffer data to the file
            size_t bytesWritten = file.write(buffer, buffer_size);

            // 3. Flush the stream and close the file to ensure data is physically written to disk
            file.flush();
            file.close();

            // 4. Verify that all bytes were successfully written
            if (bytesWritten != buffer_size)
            {
                nlog::log_errorf("SDCard", "write mismatch error! Expected %u bytes, but wrote %d.\n", va_list_t(va_t(buffer_size), va_t(bytesWritten)));
                return false;
            }

            nlog::log_infof("SDCard", "successfully wrote %u bytes to %s\n", va_list_t(va_t(buffer_size), va_t(path)));
            return true;
        }

    }  // namespace nlcd
}  // namespace ncore