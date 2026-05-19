#include <Arduino.h>

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include "ccore/c_memory.h"
#include "lib_wcs/c_spi.h"

// -------------------------------------------------------------------------------------------------
// ------ SPI --------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
namespace ncore
{
    namespace nlcd
    {
        static spi_device_handle_t g_spi_device_handle = NULL;

#define SPI_SCLK_PIN GPIO_NUM_47
#define SPI_MOSI_PIN GPIO_NUM_21
#define SPI_MISO_PIN GPIO_NUM_48

#define MY_SPI_HOST SPI2_HOST

        bool spi_is_initialized() { return g_spi_device_handle != NULL; }

        bool spi_initialize()
        {
            if (spi_is_initialized())
            {
                return true;  // SPI already initialized
            }

            spi_bus_config_t buscfg;
            ncore::g_memclr(&buscfg, sizeof(buscfg));  // Clear config structure

            buscfg.sclk_io_num     = SPI_SCLK_PIN;             // Clock pin
            buscfg.mosi_io_num     = SPI_MOSI_PIN;             // Master-out, slave-in pin
            buscfg.miso_io_num     = SPI_MISO_PIN;             // Master-in, slave-out pin
            buscfg.quadwp_io_num   = -1;                       // WP pin for Quad mode; set to -1 when unused
            buscfg.quadhd_io_num   = -1;                       // HD pin for Quad mode; set to -1 when unused
            buscfg.max_transfer_sz = 320 * 240 * sizeof(u16);  // Maximum transfer size (full screen, RGB565)

            // Initialize SPI bus
            ESP_ERROR_CHECK(spi_bus_initialize(MY_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

            // SPI device interface configuration (SPI SD card clock is typically 20-25MHz)
            spi_device_interface_config_t devcfg;
            ncore::g_memclr(&devcfg, sizeof(devcfg));  // Clear config structure
            devcfg.clock_speed_hz = 10 * 1000 * 1000;  // SPI clock
            devcfg.mode           = 0;                 // SPI mode 0
            devcfg.spics_io_num   = GPIO_NUM_NC;       // Chip-select pin
            devcfg.queue_size     = 7;                 // Transaction queue size: 7

            // Add device to SPI bus
            ESP_ERROR_CHECK(spi_bus_add_device(MY_SPI_HOST, &devcfg, &g_spi_device_handle));

            return true;
        }
    }  // namespace nlcd
}  // namespace ncore