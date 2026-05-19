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

#include "ccore/c_memory.h"
#include "lib_wcs/c_xl9555.h"

// -------------------------------------------------------------------------------------------------
// ------ XL9555 -----------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

#define XL9555_INPUT_PORT0_REG     0 /* 输入寄存器0地址 */
#define XL9555_INPUT_PORT1_REG     1 /* 输入寄存器1地址 */
#define XL9555_OUTPUT_PORT0_REG    2 /* 输出寄存器0地址 */
#define XL9555_OUTPUT_PORT1_REG    3 /* 输出寄存器1地址 */
#define XL9555_INVERSION_PORT0_REG 4 /* 极性反转寄存器0地址 */
#define XL9555_INVERSION_PORT1_REG 5 /* 极性反转寄存器1地址 */
#define XL9555_CONFIG_PORT0_REG    6 /* 方向配置寄存器0地址 */
#define XL9555_CONFIG_PORT1_REG    7 /* 方向配置寄存器1地址 */

#define XL9555_ADDR 0X20 /* XL9555器件7位地址-->请看手册（9.1. Device Address） */

#define XL9555_INPUT_REG  0x00
#define XL9555_OUTPUT_REG 0x02
#define XL9555_CONFIG_REG 0x06

#define LED1(x)                                                                                          \
    do                                                                                                   \
    {                                                                                                    \
        x ? xl9555_pin_write(ncore::nxl9555::LED1_IO, 1) : xl9555_pin_write(ncore::nxl9555::LED1_IO, 0); \
    } while (0)

#define LED1_TOGGLE() xl9555_pin_toggle(ncore::nxl9555::LED1_IO);

esp_err_t xl9555_init(void);
int16_t   xl9555_pin_read(uint16_t pin);
uint16_t  xl9555_pin_write(uint16_t pin, int val);
esp_err_t xl9555_read_byte(uint8_t reg, uint8_t *data, size_t len);
esp_err_t xl9555_write_byte(uint8_t reg, uint8_t *data, size_t len);
esp_err_t xl9555_pin_toggle(uint16_t pin);

struct device_t
{
    const char             *tag;
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    uint16_t                output_cache;
    bool                    initialized;
};

static device_t g_xl9555_dev = {"xl9555", NULL, NULL, 0xFFFF, false};

static esp_err_t xl9555_write_reg(device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    // XL9555 register writes are small (current usage is 2 data bytes).
    // Keep a tiny fixed buffer on stack to avoid heap allocation in I2C hot path.
    const size_t kMaxWritePayloadLen = 8;
    if (len > kMaxWritePayloadLen)
    {
        ESP_LOGE(dev->tag, "Write payload too large: %u", (unsigned)len);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t wr_buf[kMaxWritePayloadLen + 1];

    wr_buf[0] = reg;
    if (len > 0)
    {
        memcpy(&wr_buf[1], data, len);
    }

    return i2c_master_transmit(dev->dev_handle, wr_buf, len + 1, -1);
}

static esp_err_t xl9555_read_reg(device_t *dev, uint8_t reg, uint8_t *data, size_t len)
{
    // First, send the register address we want to read from, then read the data back
    return i2c_master_transmit_receive(dev->dev_handle, &reg, 1, data, len, -1);
}

esp_err_t xl9555_init(void)
{
    if (g_xl9555_dev.initialized)
    {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_config;
    ncore::g_memclr(&i2c_bus_config, sizeof(i2c_master_bus_config_t));  // Clear config structure
    i2c_bus_config.clk_source                   = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.i2c_port                     = (i2c_port_num_t)-1;
    i2c_bus_config.scl_io_num                   = (gpio_num_t)1;
    i2c_bus_config.sda_io_num                   = (gpio_num_t)2;
    i2c_bus_config.glitch_ignore_cnt            = 7;
    i2c_bus_config.flags.enable_internal_pullup = true;

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &g_xl9555_dev.bus_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(g_xl9555_dev.tag, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t i2c_dev_config;
    ncore::g_memclr(&i2c_dev_config, sizeof(i2c_dev_config));  // Clear config structure
    i2c_dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    i2c_dev_config.device_address  = XL9555_ADDR;
    i2c_dev_config.scl_speed_hz    = 400000;

    ret = i2c_master_bus_add_device(g_xl9555_dev.bus_handle, &i2c_dev_config, &g_xl9555_dev.dev_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(g_xl9555_dev.tag, "i2c_master_bus_add_device failed: %s", esp_err_to_name(ret));
        i2c_del_master_bus(g_xl9555_dev.bus_handle);
        g_xl9555_dev.bus_handle = NULL;
        return ret;
    }

    // Configure pins as per your maker's 0xF003 mask:
    // 0xF003 -> Port 0: 0x03 (bits 0,1 in), Port 1: 0xF0 (bits 4-7 in)
    uint8_t config[] = {0x03, 0xF0};
    ret              = xl9555_write_reg(&g_xl9555_dev, XL9555_CONFIG_REG, config, 2);
    if (ret != ESP_OK)
    {
        ESP_LOGE(g_xl9555_dev.tag, "XL9555 configuration failed: %s", esp_err_to_name(ret));
        i2c_master_bus_rm_device(g_xl9555_dev.dev_handle);
        i2c_del_master_bus(g_xl9555_dev.bus_handle);
        g_xl9555_dev.dev_handle = NULL;
        g_xl9555_dev.bus_handle = NULL;
        return ret;
    }

    g_xl9555_dev.initialized = true;
    return ESP_OK;
}

uint16_t xl9555_pin_write(uint16_t pin, int val)
{
    if (xl9555_init() != ESP_OK)
    {
        return g_xl9555_dev.output_cache;
    }

    uint16_t old_cache = g_xl9555_dev.output_cache;
    uint16_t new_cache = old_cache;

    if (val)
        new_cache |= pin;
    else
        new_cache &= ~pin;

    uint8_t data[2] = {(uint8_t)(new_cache & 0xFF), (uint8_t)((new_cache >> 8) & 0xFF)};

    if (xl9555_write_byte(XL9555_OUTPUT_REG, data, 2) == ESP_OK)
    {
        g_xl9555_dev.output_cache = new_cache;
    }
    else
    {
        ESP_LOGE(g_xl9555_dev.tag, "Failed to write output register");
    }

    return g_xl9555_dev.output_cache;
}

int16_t xl9555_pin_read(uint16_t pin)
{
    uint8_t data[2];
    if (xl9555_read_byte(XL9555_INPUT_REG, (uint8_t *)data, 2) != ESP_OK)
        return -1;

    uint16_t status = (data[1] << 8) | data[0];
    return (status & pin) ? 1 : 0;
}

esp_err_t xl9555_pin_toggle(uint16_t pin)
{
    bool currentState = (g_xl9555_dev.output_cache & pin);
    xl9555_pin_write(pin, !currentState);
    return ESP_OK;
}

esp_err_t xl9555_write_byte(uint8_t reg, uint8_t *data, size_t len)
{
    if (xl9555_init() != ESP_OK)
    {
        return ESP_FAIL;
    }

    return xl9555_write_reg(&g_xl9555_dev, reg, data, len);
}

esp_err_t xl9555_read_byte(uint8_t reg, uint8_t *data, size_t len)
{
    if (xl9555_init() != ESP_OK)
    {
        return ESP_FAIL;
    }

    return xl9555_read_reg(&g_xl9555_dev, reg, data, len);
}

namespace ncore
{
    namespace nxl9555
    {
        bool init() { return xl9555_init() == ESP_OK; }
        i16  pin_read(u16 pin) { return xl9555_pin_read(pin); }
        u16  pin_write(u16 pin, i32 val) { return xl9555_pin_write(pin, val); }
        bool read_byte(u8 reg, u8 *data, u32 len) { return xl9555_read_byte(reg, data, len) == ESP_OK; }
        bool write_byte(u8 reg, u8 *data, u32 len) { return xl9555_write_byte(reg, data, len) == ESP_OK; }
        bool pin_toggle(u16 pin) { return xl9555_pin_toggle(pin) == ESP_OK; }

    }  // namespace nxl9555
}  // namespace ncore