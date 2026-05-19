#include <Arduino.h>
#include <Wire.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_compiler.h"

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

#define LED1(x)                                                          \
    do                                                                   \
    {                                                                    \
        x ? xl9555_pin_write(ncore::nxl9555::LED1_IO, 1) : xl9555_pin_write(ncore::nxl9555::LED1_IO, 0); \
    } while (0)

#define LED1_TOGGLE() xl9555_pin_toggle(ncore::nxl9555::LED1_IO);

esp_err_t xl9555_init(void);
int16_t   xl9555_pin_read(uint16_t pin);
uint16_t  xl9555_pin_write(uint16_t pin, int val);
esp_err_t xl9555_read_byte(uint8_t reg, uint8_t *data, size_t len);
esp_err_t xl9555_write_byte(uint8_t reg, uint8_t *data, size_t len);
esp_err_t xl9555_pin_toggle(uint16_t pin);

// Cache the current output state to avoid unnecessary I2C reads
uint16_t g_xl9555_output_cache = 0xFFFF;

esp_err_t xl9555_init(void)
{
    Wire.begin(2, 1);  // SDA: 2, SCL: 1 as per your board

    // Configure pins as per your maker's 0xF003 mask:
    // 0xF003 -> Port 0: 0x03 (bits 0,1 in), Port 1: 0xF0 (bits 4-7 in)
    uint8_t config[] = {0x03, 0xF0};
    return xl9555_write_byte(XL9555_CONFIG_REG, config, 2);
}

uint16_t xl9555_pin_write(uint16_t pin, int val)
{
    if (val)
        g_xl9555_output_cache |= pin;
    else
        g_xl9555_output_cache &= ~pin;

    uint8_t data[2] = {(uint8_t)(g_xl9555_output_cache & 0xFF), (uint8_t)((g_xl9555_output_cache >> 8) & 0xFF)};

    xl9555_write_byte(XL9555_OUTPUT_REG, data, 2);
    return g_xl9555_output_cache;
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
    bool currentState = (g_xl9555_output_cache & pin);
    xl9555_pin_write(pin, !currentState);
    return ESP_OK;
}

esp_err_t xl9555_write_byte(uint8_t reg, uint8_t *data, size_t len)
{
    Wire.beginTransmission(XL9555_ADDR);
    Wire.write(reg);
    for (size_t i = 0; i < len; i++)
    {
        Wire.write(data[i]);
    }
    if (Wire.endTransmission() == 0)
        return ESP_OK;
    return ESP_FAIL;
}

esp_err_t xl9555_read_byte(uint8_t reg, uint8_t *data, size_t len)
{
    Wire.beginTransmission(XL9555_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);  // Restart
    Wire.requestFrom((uint8_t)XL9555_ADDR, (uint8_t)len);

    size_t i = 0;
    while (Wire.available() && i < len)
    {
        data[i++] = Wire.read();
    }
    return (i == len) ? ESP_OK : ESP_FAIL;
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