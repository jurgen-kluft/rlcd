#include <Arduino.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lib_wcs/c_i2c.h"

// -------------------------------------------------------------------------------------------------
// ------ I2C --------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
#define IIC_NUM_PORT  I2C_NUM_0  // I2C port number
#define IIC_SPEED_CLK 400000     // I2C clock speed (400kHz)

namespace ncore
{
    namespace ni2c
    {

        struct i2c_master_dev_t
        {
            i2c_master_bus_handle_t bus;  // I2C bus handle
            u8                      sda;
            u8                      scl;
        };

        i2c_master_dev_t* i2c_master_dev_create(i2c_master_bus_handle_t bus, u8 sda, u8 scl)
        {
            i2c_master_dev_t* dev = (i2c_master_dev_t*)malloc(sizeof(i2c_master_dev_t));
            if (dev == NULL)
            {
                ESP_LOGE("I2C", "Failed to allocate memory for I2C device");
                return NULL;
            }

            i2c_master_bus_config_t i2c_bus_config;
            i2c_bus_config.clk_source                   = I2C_CLK_SRC_DEFAULT;  // Clock source
            i2c_bus_config.i2c_port                     = IIC_NUM_PORT;         // I2C port
            i2c_bus_config.scl_io_num                   = (gpio_num_t)scl;      // SCL pin
            i2c_bus_config.sda_io_num                   = (gpio_num_t)sda;      // SDA pin
            i2c_bus_config.glitch_ignore_cnt            = 7;                    // Glitch filter count
            i2c_bus_config.flags.enable_internal_pullup = true;                 // Enable internal pull-up

            ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &dev->bus));

            dev->sda = sda;
            dev->scl = scl;

            return dev;
        }
    }  // namespace ni2c
}  // namespace ncore