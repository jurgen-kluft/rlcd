#include <Arduino.h>

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include "rcore/c_log.h"
#include "ccore/c_memory.h"

#include "lib_wcs/c_spi.h"
#include "lib_wcs/c_sdcard.h"
#include "lib_wcs/c_xl9555.h"

// -------------------------------------------------------------------------------------------------
// ------ SD Card SPI ------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
namespace ncore
{
    namespace nlcd
    {
        struct sdcard_device_t
        {
            sdmmc_card_t *card;
            const char   *mount_point;
            esp_err_t     ret;
            esp_err_t     mount_ret;
        };
        static sdcard_device_t g_sdcard_dev;

#define SD_CS(x)                                                                                                             \
    do                                                                                                                       \
    {                                                                                                                        \
        x ? ncore::nxl9555::pin_write(ncore::nxl9555::SD_CS_IO, 1) : ncore::nxl9555::pin_write(ncore::nxl9555::SD_CS_IO, 0); \
    } while (0)

        bool sdcard_initialize()
        {
            bool result = true;

            g_sdcard_dev.card        = nullptr;
            g_sdcard_dev.mount_point = "/0:";
            g_sdcard_dev.ret         = ESP_OK;
            g_sdcard_dev.mount_ret   = ESP_FAIL;

            if (ncore::nlcd::spi_is_initialized())
            {
                if (g_sdcard_dev.mount_ret == ESP_OK)
                {
                    nlog::println("SDCard: card already mounted, unmounting first");
                    esp_vfs_fat_sdcard_unmount(g_sdcard_dev.mount_point, g_sdcard_dev.card);  // Unmount
                    g_sdcard_dev.mount_ret = ESP_FAIL;
                }
            }

            if (!ncore::nlcd::spi_initialize())
            {
                nlog::println("SDCard: Failed to initialize SPI bus");
                return false;
            }

            esp_vfs_fat_sdmmc_mount_config_t mount_config;
            ncore::g_memclr(&mount_config, sizeof(mount_config));  // Clear config structure
            mount_config.format_if_mount_failed = true;
            mount_config.max_files              = 5;
            mount_config.allocation_unit_size   = 16 * 1024;  // 16 KB allocation unit size for better performance on SPI flash

            // SD card parameter configuration
            sdmmc_host_t host = SDSPI_HOST_DEFAULT();

            // SD card pin configuration
            sdspi_device_config_t slot_config;
            ncore::g_memclr(&slot_config, sizeof(sdspi_device_config_t));  // Clear config structure
            slot_config.host_id  = (spi_host_device_t)host.slot;
            slot_config.gpio_cs  = GPIO_NUM_NC;
            slot_config.gpio_cd  = GPIO_NUM_NC;
            slot_config.gpio_wp  = GPIO_NUM_NC;
            slot_config.gpio_int = GPIO_NUM_NC;

            SD_CS(0);
            g_sdcard_dev.mount_ret = esp_vfs_fat_sdspi_mount(g_sdcard_dev.mount_point, &host, &slot_config, &mount_config, &g_sdcard_dev.card);  // Mount file system
            SD_CS(1);
            if (g_sdcard_dev.mount_ret != ESP_OK)
            {
                nlog::printfln("SDCard: Failed to mount SD card (error code: %d)", va_t(g_sdcard_dev.mount_ret));
                result = false;
            }
            vTaskDelay(pdMS_TO_TICKS(10));

            return result;
        }

        bool sdcard_get_usage(u64 *out_total_bytes, u64 *out_free_bytes)
        {
            FATFS  *fs;
            DWORD   free_clusters;
            FRESULT res = f_getfree("0:", (DWORD *)&free_clusters, &fs);
            if (res == FR_OK)
            {
                size_t total_sectors = (fs->n_fatent - 2) * fs->csize;
                size_t free_sectors  = free_clusters * fs->csize;

                size_t sd_total    = total_sectors / 1024;
                size_t sd_total_KB = sd_total * fs->ssize;
                size_t sd_free     = free_sectors / 1024;
                size_t sd_free_KB  = sd_free * fs->ssize;

                // Assume total size is less than 4 GiB, which should be true for SPI flash
                if (out_total_bytes != NULL)
                {
                    *out_total_bytes = sd_total_KB;
                }

                if (out_free_bytes != NULL)
                {
                    *out_free_bytes = sd_free_KB;
                }
                return true;
            }
            else
            {
                nlog::printfln("SDCard: Failed to get SD card free space (error code: %d)", va_t((u32)res));
                return false;
            }
        }
    }  // namespace nlcd
}  // namespace ncore