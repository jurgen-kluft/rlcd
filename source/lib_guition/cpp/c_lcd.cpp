#include <Arduino.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"

#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "ccore/c_memory.h"

#include "esp_lcd_st7701/esp_lcd_st7701.h"
#include "esp_lcd_touch_gt911/esp_lcd_touch_gt911.h"
#include "esp_lcd_panel_io_additions/esp_lcd_panel_io_additions.h"

#include "lib_guition/c_lcd.h"

/* =========================================================
 *                  DEFINES
 * ========================================================= */
#define TAG "GUITION"

/* =========================================================
 *                  LCD BACKLIGHT
 * ========================================================= */

/**
 * @defgroup GUITION_LCD_BACKLIGHT LCD Backlight
 * @brief LCD backlight control configuration
 * @{
 */

#define GUITION_LCD_BACKLIGHT_GPIO    38
#define GUITION_LCD_BACKLIGHT_LEDC_CH LEDC_CHANNEL_0
/** @} */  // end of GUITION_LCD_BACKLIGHT

/* =========================================================
 *                  LCD SPI PINS
 * ========================================================= */

/**
 * @defgroup GUITION_LCD_SPI LCD SPI Configuration
 * @brief 3-wire SPI interface for LCD
 * @{
 */

#define GUITION_LCD_SPI_CS_GPIO   39
#define GUITION_LCD_SPI_SCK_GPIO  48
#define GUITION_LCD_SPI_MOSI_GPIO 47

/** @} */  // end of GUITION_LCD_SPI

/* =========================================================
 *                  LCD RGB PINS
 * ========================================================= */

/**
 * @defgroup GUITION_LCD_RGB LCD RGB Interface
 * @brief RGB panel data and control lines
 * @{
 */

#define GUITION_LCD_DE_GPIO    18
#define GUITION_LCD_HSYNC_GPIO 16
#define GUITION_LCD_VSYNC_GPIO 17
#define GUITION_LCD_PCLK_GPIO  21
#define GUITION_LCD_DISP_GPIO  GPIO_NUM_NC
#define GUITION_LCD_RST_GPIO   GPIO_NUM_NC

/** @defgroup GUITION_LCD_RGB_RED Red Channel
 *  @{
 */
#define GUITION_LCD_R0_GPIO 11
#define GUITION_LCD_R1_GPIO 12
#define GUITION_LCD_R2_GPIO 13
#define GUITION_LCD_R3_GPIO 14
#define GUITION_LCD_R4_GPIO 0
/** @} */

/** @defgroup GUITION_LCD_RGB_GREEN Green Channel
 *  @{
 */
#define GUITION_LCD_G0_GPIO 8
#define GUITION_LCD_G1_GPIO 20
#define GUITION_LCD_G2_GPIO 3
#define GUITION_LCD_G3_GPIO 46
#define GUITION_LCD_G4_GPIO 9
#define GUITION_LCD_G5_GPIO 10
/** @} */

/** @defgroup GUITION_LCD_RGB_BLUE Blue Channel
 *  @{
 */
#define GUITION_LCD_B0_GPIO 4
#define GUITION_LCD_B1_GPIO 5
#define GUITION_LCD_B2_GPIO 6
#define GUITION_LCD_B3_GPIO 7
#define GUITION_LCD_B4_GPIO 15
/** @} */

/** @} */  // end of GUITION_LCD_RGB

/* =========================================================
 *                  LCD PANEL CONFIGURATION
 * ========================================================= */

/**
 * @defgroup GUITION_LCD_PANEL LCD Panel Configuration
 * @brief Display resolution and bit depth
 * @{
 */

#define GUITION_LCD_H_RES          480
#define GUITION_LCD_V_RES          480
#define GUITION_LCD_BITS_PER_PIXEL 16

/** @} */  // end of GUITION_LCD_PANEL

/* =========================================================
 *                  TOUCH PANEL PINS
 * ========================================================= */

/**
 * @defgroup GUITION_TOUCH Touch Panel
 * @brief GT911 touch controller interface
 * @{
 */

#define GUITION_TOUCH_I2C_SCL_GPIO 45
#define GUITION_TOUCH_I2C_SDA_GPIO 19
#define GUITION_TOUCH_INT_GPIO     GPIO_NUM_NC
#define GUITION_TOUCH_RST_GPIO     GPIO_NUM_NC
#define GUITION_TOUCH_I2C_CLK_HZ   400000

/** @} */  // end of GUITION_TOUCH

/* =========================================================
 *                  CONSTANTS
 * ========================================================= */

/**
 * @brief ST7701 initialization command sequence
 *
 * This sequence configures the ST7701 LCD controller for 480x480
 * RGB panel operation with proper voltage levels and gamma correction.
 */
static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
  {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x13}, 5, 0},
  {0xEF, (uint8_t[]){0x08}, 1, 0},

  {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
  {0xC0, (uint8_t[]){0x3B, 0x00}, 2, 0},
  {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
  {0xC2, (uint8_t[]){0x21, 0x08}, 2, 0},
  {0xCD, (uint8_t[]){0x00}, 1, 0},
  {0xB0, (uint8_t[]){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}, 16, 0},
  {0xB1, (uint8_t[]){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}, 16, 0},

  {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
  {0xB0, (uint8_t[]){0x60}, 1, 0},
  {0xB1, (uint8_t[]){0x30}, 1, 0},
  {0xB2, (uint8_t[]){0x87}, 1, 0},
  {0xB3, (uint8_t[]){0x80}, 1, 0},
  {0xB5, (uint8_t[]){0x49}, 1, 0},
  {0xB7, (uint8_t[]){0x85}, 1, 0},
  {0xB8, (uint8_t[]){0x21}, 1, 0},
  {0xC1, (uint8_t[]){0x78}, 1, 0},
  {0xC2, (uint8_t[]){0x78}, 1, 20},
  {0xE0, (uint8_t[]){0x00, 0x1B, 0x02}, 3, 0},
  {0xE1, (uint8_t[]){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}, 11, 0},
  {0xE2, (uint8_t[]){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}, 12, 0},
  {0xE3, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
  {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
  {0xE5, (uint8_t[]){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}, 16, 0},
  {0xE6, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
  {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
  {0xE8, (uint8_t[]){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}, 16, 0},
  {0xEB, (uint8_t[]){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7, 0},
  {0xEC, (uint8_t[]){0x3C, 0x00}, 2, 0},
  {0xED, (uint8_t[]){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}, 16, 0},

  {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},

  {0x11, NULL, 0, 120},
  {0x29, NULL, 0, 0},
};

/* =========================================================
 *                  STATIC VARIABLES
 * ========================================================= */
static esp_lcd_panel_handle_t g_panel_handle = NULL;

struct lcd_obj_t
{
    uint16_t width;
    uint16_t height;
    uint16_t pwidth;
    uint16_t pheight;
    uint8_t  dir;
    uint16_t wramcmd;
    uint16_t setxcmd;
    uint16_t setycmd;
    uint16_t wr;
    uint16_t cs;
    uint16_t dc;
    uint16_t rd;
};

static lcd_obj_t g_lcd_dev;

/* =========================================================
 *  Initialize LCD backlight brightness control
 * ========================================================= */

esp_err_t guition_display_brightness_init(void)
{
    ESP_LOGI(TAG, "Initializing LCD backlight brightness control");

    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t LCD_backlight_channel = {
      .gpio_num = GUITION_LCD_BACKLIGHT_GPIO, .speed_mode = LEDC_LOW_SPEED_MODE, .channel = GUITION_LCD_BACKLIGHT_LEDC_CH, .intr_type = LEDC_INTR_DISABLE, .timer_sel = (ledc_timer_t)1, .duty = 0, .hpoint = 0};
    const ledc_timer_config_t LCD_backlight_timer = {.speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_10_BIT, .timer_num = (ledc_timer_t)1, .freq_hz = 200, .clk_cfg = LEDC_AUTO_CLK};

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));

    return ESP_OK;
}

/* ================================================
 *  Initialize LCD panel
 * ================================================ */
esp_err_t guition_display_init(void)
{
    ESP_LOGI(TAG, "Initializing 3-wire SPI");

    g_lcd_dev.width   = GUITION_LCD_H_RES;
    g_lcd_dev.height  = GUITION_LCD_V_RES;
    g_lcd_dev.pwidth  = GUITION_LCD_H_RES;
    g_lcd_dev.pheight = GUITION_LCD_V_RES;

    g_lcd_dev.dir     = 0;
    g_lcd_dev.wramcmd = 0x2C;
    g_lcd_dev.setxcmd = 0x2A;
    g_lcd_dev.setycmd = 0x2B;

    g_lcd_dev.wr = 0;
    g_lcd_dev.cs = 0;
    g_lcd_dev.dc = 0;
    g_lcd_dev.rd = 0;

    // Configure SPI line: CS, SCL (clock), SDA (data)
    spi_line_config_t line_config;
    ncore::g_memset(&line_config, 0, sizeof(line_config));
    line_config.cs_io_type   = IO_TYPE_GPIO;
    line_config.cs_gpio_num  = GUITION_LCD_SPI_CS_GPIO;
    line_config.scl_io_type  = IO_TYPE_GPIO;
    line_config.scl_gpio_num = GUITION_LCD_SPI_SCK_GPIO;
    line_config.sda_io_type  = IO_TYPE_GPIO;
    line_config.sda_gpio_num = GUITION_LCD_SPI_MOSI_GPIO;
    line_config.io_expander  = NULL;

    esp_lcd_panel_io_3wire_spi_config_t io_config;
    ST7701_PANEL_IO_3WIRE_SPI_CONFIG(io_config, line_config, 0);

    esp_lcd_panel_io_handle_t io_handle = NULL;

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle), TAG, "Failed to create panel IO: %s");

    ESP_LOGI(TAG, "Initializing ST7701 driver");
    esp_lcd_rgb_panel_config_t rgb_config;
    rgb_config.clk_src = LCD_CLK_SRC_PLL160M;
    ST7701_480_480_PANEL_60HZ_RGB_TIMING(rgb_config.timings);
    rgb_config.data_width                = 16;
    rgb_config.bits_per_pixel            = 16;
    rgb_config.dma_burst_size            = 64;
    rgb_config.hsync_gpio_num            = GUITION_LCD_HSYNC_GPIO;
    rgb_config.vsync_gpio_num            = GUITION_LCD_VSYNC_GPIO;
    rgb_config.de_gpio_num               = GUITION_LCD_DE_GPIO;
    rgb_config.pclk_gpio_num             = GUITION_LCD_PCLK_GPIO;
    rgb_config.disp_gpio_num             = GUITION_LCD_DISP_GPIO;
    rgb_config.data_gpio_nums[0]         = GUITION_LCD_R0_GPIO;
    rgb_config.data_gpio_nums[1]         = GUITION_LCD_R1_GPIO;
    rgb_config.data_gpio_nums[2]         = GUITION_LCD_R2_GPIO;
    rgb_config.data_gpio_nums[3]         = GUITION_LCD_R3_GPIO;
    rgb_config.data_gpio_nums[4]         = GUITION_LCD_R4_GPIO;
    rgb_config.data_gpio_nums[5]         = GUITION_LCD_G0_GPIO;
    rgb_config.data_gpio_nums[6]         = GUITION_LCD_G1_GPIO;
    rgb_config.data_gpio_nums[7]         = GUITION_LCD_G2_GPIO;
    rgb_config.data_gpio_nums[8]         = GUITION_LCD_G3_GPIO;
    rgb_config.data_gpio_nums[9]         = GUITION_LCD_G4_GPIO;
    rgb_config.data_gpio_nums[10]        = GUITION_LCD_G5_GPIO;
    rgb_config.data_gpio_nums[11]        = GUITION_LCD_B0_GPIO;
    rgb_config.data_gpio_nums[12]        = GUITION_LCD_B1_GPIO;
    rgb_config.data_gpio_nums[13]        = GUITION_LCD_B2_GPIO;
    rgb_config.data_gpio_nums[14]        = GUITION_LCD_B3_GPIO;
    rgb_config.data_gpio_nums[15]        = GUITION_LCD_B4_GPIO;
    rgb_config.flags.disp_active_low     = 0;
    rgb_config.flags.refresh_on_demand   = 0;
    rgb_config.flags.fb_in_psram         = 1;
    rgb_config.flags.double_fb           = 0;
    rgb_config.flags.no_fb               = 0;
    rgb_config.flags.bb_invalidate_cache = 0;
    rgb_config.num_fbs                   = 2;
    rgb_config.bounce_buffer_size_px     = GUITION_LCD_H_RES * 10;

    st7701_vendor_config_t vendor_config;
    vendor_config.rgb_config                = &rgb_config;
    vendor_config.init_cmds                 = lcd_init_cmds;
    vendor_config.init_cmds_size            = sizeof(lcd_init_cmds) / sizeof(st7701_lcd_init_cmd_t);
    vendor_config.flags.mirror_by_cmd       = 1;
    vendor_config.flags.enable_io_multiplex = 0;

    esp_lcd_panel_dev_config_t panel_config;
    panel_config.reset_gpio_num = GUITION_LCD_RST_GPIO;
    panel_config.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_BGR;
    panel_config.bits_per_pixel = GUITION_LCD_BITS_PER_PIXEL;
    panel_config.vendor_config  = &vendor_config;

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7701(io_handle, &panel_config, &g_panel_handle), TAG, "Failed to create panel: %s");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(g_panel_handle), TAG, "Failed to reset panel: %s");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(g_panel_handle), TAG, "Failed to initialize panel: %s");

    return ESP_OK;
}

/* ================================================
 *  Initialize touch panel
 * ================================================ */
esp_err_t guition_touch_init(void)
{
    ESP_LOGI(TAG, "Initializing touch panel");

    // Create I2C master bus for touch controller
    i2c_master_bus_handle_t tp_bus_handle = NULL;

    // Configure I2C: 400 kHz, internal pull-ups enabled
    i2c_master_bus_config_t i2c_mst_config;
    i2c_mst_config.clk_source                   = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port                     = -1;
    i2c_mst_config.scl_io_num                   = (gpio_num_t)GUITION_TOUCH_I2C_SCL_GPIO;
    i2c_mst_config.sda_io_num                   = (gpio_num_t)GUITION_TOUCH_I2C_SDA_GPIO;
    i2c_mst_config.glitch_ignore_cnt            = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_mst_config, &tp_bus_handle), TAG, "Failed to create I2C master bus: %s");

    esp_lcd_panel_io_i2c_config_t tp_io_config;
    ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG(tp_io_config);
    tp_io_config.scl_speed_hz = GUITION_TOUCH_I2C_CLK_HZ;

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(tp_bus_handle, &tp_io_config, &tp_io_handle), TAG, "Failed to create touch IO: %s");

    esp_lcd_touch_io_gt911_config_t tp_gt911_config;
    tp_gt911_config.dev_addr = tp_io_config.dev_addr;

    esp_lcd_touch_config_t tp_cfg;
    tp_cfg.x_max            = GUITION_LCD_V_RES;
    tp_cfg.y_max            = GUITION_LCD_H_RES;
    tp_cfg.rst_gpio_num     = (gpio_num_t)GUITION_TOUCH_RST_GPIO;
    tp_cfg.int_gpio_num     = (gpio_num_t)GUITION_TOUCH_INT_GPIO;
    tp_cfg.levels.reset     = 0;
    tp_cfg.levels.interrupt = 0;
    tp_cfg.flags.swap_xy    = 0;
    tp_cfg.flags.mirror_x   = 0;
    tp_cfg.flags.mirror_y   = 0;
    tp_cfg.driver_data      = &tp_gt911_config;

    esp_lcd_touch_handle_t tp = NULL;
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp), TAG, "Failed to create touch controller: %s");

    return ESP_OK;
}

// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
static uint16_t g_line_buffer[GUITION_LCD_H_RES] __attribute__((aligned(64)));

static void s_fill_rectangle(esp_lcd_panel_handle_t panel_handle, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    if (sx >= g_lcd_dev.width || sy >= g_lcd_dev.height || ex >= g_lcd_dev.width || ey >= g_lcd_dev.height || sx > ex || sy > ey)
    {
        ESP_LOGE(LCD_TAG, "Invalid fill area: sx=%d, sy=%d, ex=%d, ey=%d", sx, sy, ex, ey);
        return;
    }

    uint16_t width  = ex - sx + 1;
    uint16_t height = ey - sy + 1;

    for (uint16_t i = 0; i < width; i++)
    {
        g_line_buffer[i] = color;
    }

    for (uint16_t y = 0; y < height; y++)
    {
        esp_lcd_panel_draw_bitmap(panel_handle, sx, sy + y, ex + 1, sy + y + 1, g_line_buffer);
    }
}

static void s_draw_rectangle(esp_lcd_panel_handle_t panel_handle, uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    if (sx >= g_lcd_dev.width || sy >= g_lcd_dev.height || ex >= g_lcd_dev.width || ey >= g_lcd_dev.height || sx > ex || sy > ey)
    {
        ESP_LOGE("LCD_TAG", "Invalid sprite area: sx=%d, sy=%d, ex=%d, ey=%d", sx, sy, ex, ey);
        return;
    }

    uint16_t width  = ex - sx + 1;
    uint16_t height = ey - sy + 1;

    uint16_t *buffer = g_line_buffer;
    for (uint16_t i = 0; i < width; i++)
    {
        buffer[i] = color;
    }

    for (uint16_t y = 0; y < height; y++)
    {
        esp_lcd_panel_draw_bitmap(panel_handle, sx, sy + y, ex + 1, sy + y + 1, buffer);
    }
}

/* =========================================================
 *                  BACKLIGHT CONTROL API
 * ========================================================= */

esp_err_t guition_display_brightness_set(uint8_t brightness_percent)
{
    if (brightness_percent > 100)
    {
        brightness_percent = 100;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);
    uint32_t duty_cycle = (1023 * (brightness_percent)) / 100;  // LEDC resolution set to 10bits, thus: 100% = 1023
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, GUITION_LCD_BACKLIGHT_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, GUITION_LCD_BACKLIGHT_LEDC_CH));

    return ESP_OK;
}

esp_err_t guition_display_backlight_off(void) { return guition_display_brightness_set(0); }
esp_err_t guition_display_backlight_on(void) { return guition_display_brightness_set(100); }

namespace ncore
{
    namespace nlcd
    {
        static i8 g_led_brightness = 100;

        bool initialize()
        {
            guition_display_init();
            guition_display_brightness_init();
            guition_touch_init();

            ledcAttach(0, 600, 8);

            return true;
        }

        u16 width() { return GUITION_LCD_H_RES; }
        u16 height() { return GUITION_LCD_V_RES; }

        void draw_rectangle(u16 sx, u16 sy, u16 ex, u16 ey, u16 color) { s_fill_rectangle(g_panel_handle, sx, sy, ex, ey, color); }
        void draw_sprite(u16 sx, u16 sy, u16 ex, u16 ey, const u16 *color) { s_draw_rectangle(g_panel_handle, sx, sy, ex, ey, *color); }

        void led_toggle()
        {
            g_led_brightness = (g_led_brightness == 0) ? 100 : 0;
            ledcWrite(0, g_led_brightness);
        }

        void led_switch(bool on)
        {
            g_led_brightness = on ? 100 : 0;
            ledcWrite(0, g_led_brightness);
        }

        // display has a backlight control pin, which can be used to turn on/off the backlight
        void backlight_switch(bool on)
        {
            if (on)
            {
                guition_display_backlight_on();
            }
            else
            {
                guition_display_backlight_off();
            }
        }

        // display can be turned on/off
        void display_power(bool on)
        {
            if (on)
            {
                guition_display_backlight_on();
            }
            else
            {
                guition_display_backlight_off();
            }
        }

    }  // namespace nlcd
}  // namespace ncore
