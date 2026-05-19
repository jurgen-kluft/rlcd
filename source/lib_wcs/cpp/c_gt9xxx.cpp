#include <Arduino.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_compiler.h"

#include "lib_wcs/c_xl9555.h"
#include "lib_wcs/c_gt9xxx.h"

// -------------------------------------------------------------------------------------------------
// ------ GT9XXX -----------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
namespace ncore
{
    namespace ngt9xxx
    {

// Touchscreen reset pin
#define CT_RST(x)                                                                                                              \
    do                                                                                                                         \
    {                                                                                                                          \
        x ? ncore::nxl9555::pin_write(ncore::nxl9555::TP_RST_IO, 1) : ncore::nxl9555::pin_write(ncore::nxl9555::TP_RST_IO, 0); \
    } while (0)

// Touch controller pin definitions
#define GT9XXX_INT_GPIO_PIN GPIO_NUM_42

#define GT9XXX_INT gpio_get_level(GT9XXX_INT_GPIO_PIN)

        struct device_t
        {
            const char             *tag;  // = "gt9xxx";
            i2c_master_bus_handle_t myiic1_bus_handle;
            i2c_master_dev_handle_t gt9xxx_handle;

            // Note: except for GT9271 (10-point touch), other touch ICs support only 5 points.
            u8 max_touch_points;  // Default number of supported touch points (5-point touch)
        };

        static device_t g_gt9xxx_dev = {"gt9xxx", NULL, NULL, 5};

        // Get the maximum number of touch points supported by the device
        u8 max_tps() { return g_gt9xxx_dev.max_touch_points; }

        // Write data to GT9XXX
        // reg: start register address
        // buf: data buffer
        // len: number of bytes to write
        // return: esp_err_t (ESP_OK on success)
        static esp_err_t gt9xxx_wr_reg(device_t *dev, u16 reg, u8 *buf, u8 len)
        {
            esp_err_t ret;
            u8       *wr_buf = (u8 *)malloc(2 + len);

            if (wr_buf == NULL)
            {
                ESP_LOGE(dev->tag, "%s memory failed", __func__);
                return ESP_ERR_NO_MEM;  // Memory allocation failed
            }

            wr_buf[0] = reg >> 8;
            wr_buf[1] = reg & 0XFF;

            memcpy(wr_buf + 2, buf, len);  // Copy data to TX buffer

            ret = i2c_master_transmit(dev->gt9xxx_handle, wr_buf, 2 + len, -1);

            free(wr_buf);  // Release memory after transmission

            return ret;
        }

        // Read data from GT9XXX
        // reg: start register address
        // buf: data buffer
        // len: number of bytes to read
        // return: esp_err_t (ESP_OK on success)
        static esp_err_t gt9xxx_rd_reg(device_t *dev, u16 reg, u8 *buf, u8 len)
        {
            u8 memaddr_buf[2];
            memaddr_buf[0] = reg >> 8;
            memaddr_buf[1] = reg & 0XFF;

            return i2c_master_transmit_receive(dev->gt9xxx_handle, memaddr_buf, sizeof(memaddr_buf), buf, len, -1);
        }

        // Initialize GT9XXX touchscreen
        // return: ESP_OK on success, non-zero on failure
        bool init()
        {
            u8 temp[5];

            device_t *dev = &g_gt9xxx_dev;
            dev->tag      = "gt9xxx";

            i2c_master_bus_config_t i2c1_bus_config;
            i2c1_bus_config.clk_source                   = I2C_CLK_SRC_DEFAULT;  // Clock source
            i2c1_bus_config.i2c_port                     = I2C_NUM_1;            // I2C port
            i2c1_bus_config.scl_io_num                   = GPIO_NUM_40;          // SCL pin
            i2c1_bus_config.sda_io_num                   = GPIO_NUM_41;          // SDA pin
            i2c1_bus_config.glitch_ignore_cnt            = 7;                    // Glitch filter count
            i2c1_bus_config.flags.enable_internal_pullup = true;                 // Enable internal pull-up

            // Create a new I2C bus
            ESP_ERROR_CHECK(i2c_new_master_bus(&i2c1_bus_config, &dev->myiic1_bus_handle));

            i2c_device_config_t gt9xxx_i2c_dev_conf;
            gt9xxx_i2c_dev_conf.dev_addr_length = I2C_ADDR_BIT_LEN_7;  // Slave address length
            gt9xxx_i2c_dev_conf.scl_speed_hz    = 400000;              // Transfer speed
            gt9xxx_i2c_dev_conf.device_address  = GT9XXX_DEV_ID;       // 7-bit slave address

            // Add GT9XXX device to the I2C bus
            ESP_ERROR_CHECK(i2c_master_bus_add_device(dev->myiic1_bus_handle, &gt9xxx_i2c_dev_conf, &dev->gt9xxx_handle));

            // Configure CT_RST and CT_INT pins; timing determines IC address 0x14
            // (some chips support an alternate timing sequence for address 0x5D).
            gpio_config_t gpio_init_struct = {0};
            gpio_init_struct.intr_type     = GPIO_INTR_DISABLE;            // Disable pin interrupt
            gpio_init_struct.pull_up_en    = GPIO_PULLUP_DISABLE;          // Disable pull-up
            gpio_init_struct.pull_down_en  = GPIO_PULLDOWN_DISABLE;        // Disable pull-down
            gpio_init_struct.mode          = GPIO_MODE_INPUT;              // Input mode
            gpio_init_struct.pin_bit_mask  = 1ull << GT9XXX_INT_GPIO_PIN;  // Pin bitmask
            gpio_config(&gpio_init_struct);                                // Configure interrupt pin

            for (int i = 2; i > 0; i--)
            {
                CT_RST(0);
                vTaskDelay(pdMS_TO_TICKS(200));
                CT_RST(1);
                vTaskDelay(pdMS_TO_TICKS(200));
            }

            vTaskDelay(pdMS_TO_TICKS(100));

            gt9xxx_rd_reg(dev, GT9XXX_PID_REG, temp, 4);  // Read product ID
            temp[4] = 0;

            // Check whether this is a supported touch controller
            const char *supported_ids[]   = {"911", "9147", "1158", "9271", "928"};
            const u16   supported_tnums[] = {5, 5, 5, 10, 10};
            i32         supported_id      = -1;
            for (i32 i = 0; i < sizeof(supported_ids) / sizeof(supported_ids[0]); i++)
            {
                if (strcmp((char *)temp, supported_ids[i]) == 0)
                {
                    dev->max_touch_points = supported_tnums[i];  // Set the number of touch points based on the ID
                    supported_id          = i;
                    break;
                }
            }
            if (supported_id == -1)
            {
                free(dev);
                return NULL;  // Init failed: unsupported IC. Check touch IC model and timing sequence
            }
            ESP_LOGI("GT9XXX", "CTP:%s", supported_ids[supported_id]);  // Print touch controller IC ID

            temp[0] = 0X02;
            gt9xxx_wr_reg(dev, GT9XXX_CTRL_REG, temp, 1);  // Soft-reset GT9XXX

            vTaskDelay(10);

            temp[0] = 0X00;
            gt9xxx_wr_reg(dev, GT9XXX_CTRL_REG, temp, 1);  // End reset and enter coordinate-read mode

            return ESP_OK;
        }

        bool write_reg(u16 reg, u8 *buf, u8 len) { return gt9xxx_wr_reg(&g_gt9xxx_dev, reg, buf, len) == ESP_OK; }
        bool read_reg(u16 reg, u8 *buf, u8 len) { return gt9xxx_rd_reg(&g_gt9xxx_dev, reg, buf, len) == ESP_OK; }

        // Register table for up to 10 GT9XXX touch points
        static const u16 GT9XXX_TPX_TBL[10] = {
          GT9XXX_TP1_REG, GT9XXX_TP2_REG, GT9XXX_TP3_REG, GT9XXX_TP4_REG, GT9XXX_TP5_REG, GT9XXX_TP6_REG, GT9XXX_TP7_REG, GT9XXX_TP8_REG, GT9XXX_TP9_REG, GT9XXX_TP10_REG,
        };
        bool read_tpx_reg(u16 i, u8 *buf, u8 len)
        {
            if (i >= g_gt9xxx_dev.max_touch_points)
            {
                ESP_LOGE(g_gt9xxx_dev.tag, "Invalid touch point index: %d", i);
                return false;  // Invalid touch point index
            }
            return gt9xxx_rd_reg(&g_gt9xxx_dev, GT9XXX_TPX_TBL[i], buf, len) == ESP_OK;
        }

    }  // namespace ngt9xxx
}  // namespace ncore