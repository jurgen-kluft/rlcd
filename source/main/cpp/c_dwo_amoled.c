#include <stdlib.h>
#include <sys/cdefs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"

#include "rdno_lcd/private/esp_lcd_sh8601.h"
#include "rdno_lcd/private/esp_lcd_touch_ft5x06.h"

static const char* TAG = "rdno_lcd";

#define LCD_HOST   SPI2_HOST
#define TOUCH_HOST I2C_NUM_0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_LCD_CS         (GPIO_NUM_9)
#define EXAMPLE_PIN_NUM_LCD_PCLK       (GPIO_NUM_10)
#define EXAMPLE_PIN_NUM_LCD_DATA0      (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1      (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2      (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3      (GPIO_NUM_14)
#define EXAMPLE_PIN_NUM_LCD_RST        (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT       (-1)

#define EXAMPLE_USE_TOUCH 1

#if EXAMPLE_USE_TOUCH
    #define EXAMPLE_PIN_NUM_TOUCH_SCL (GPIO_NUM_48)
    #define EXAMPLE_PIN_NUM_TOUCH_SDA (GPIO_NUM_47)
    #define EXAMPLE_PIN_NUM_TOUCH_RST (GPIO_NUM_3)
    #define EXAMPLE_PIN_NUM_TOUCH_INT (-1)

esp_lcd_touch_handle_t tp = NULL;
#endif

static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
  // 2.06 inch AMOLED from DWO LIMITED
  // Part number: DO0206FMST01-QSPI | DO0206PFST02-QSPI |  DO0206FMST03-QSPI
  // Size: 2.06 inch
  // Resolution: 410x502
  // Signal interface:  QSPI
  // For more product information, please visit www.dwo.net.cn

  {0x11, (uint8_t[]){0x00}, 0, 120},
  {0xC4, (uint8_t[]){0x80}, 1, 0},
  {0x44, (uint8_t[]){0x01, 0xD1}, 2, 0},
  {0x35, (uint8_t[]){0x00}, 1, 0},
  {0x53, (uint8_t[]){0x20}, 1, 10},
  {0x63, (uint8_t[]){0xFF}, 1, 10},
  {0x51, (uint8_t[]){0x00}, 1, 10},
  {0x2A, (uint8_t[]){0x00, 0x16, 0x01, 0xAF}, 4, 0},
  {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xF5}, 4, 0},
  {0x29, (uint8_t[]){0x00}, 0, 10},
  {0x51, (uint8_t[]){0xFF}, 1, 0},
};

void init_dwo_display(int32_t h_res, int32_t v_res, int32_t bpp, esp_lcd_panel_io_color_trans_done_cb_t notify_flush_ready, void* notify_flush_ready_user_ctx)
{
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = SH8601_PANEL_BUS_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_PCLK, EXAMPLE_PIN_NUM_LCD_DATA0, EXAMPLE_PIN_NUM_LCD_DATA1, EXAMPLE_PIN_NUM_LCD_DATA2, EXAMPLE_PIN_NUM_LCD_DATA3, h_res * v_res * bpp / 8);
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t           io_handle     = NULL;
    const esp_lcd_panel_io_spi_config_t io_config     = SH8601_PANEL_IO_QSPI_CONFIG(EXAMPLE_PIN_NUM_LCD_CS, notify_flush_ready, notify_flush_ready_user_ctx);
    sh8601_vendor_config_t              vendor_config = {
                   .init_cmds      = lcd_init_cmds,
                   .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
                   .flags =
        {
                       .use_qspi_interface = 1,
        },
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t           panel_handle = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
      .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = bpp,
      .vendor_config  = &vendor_config,
    };
    ESP_LOGI(TAG, "Install SH8601 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if EXAMPLE_USE_TOUCH
    ESP_LOGI(TAG, "Initialize I2C bus");
    const i2c_config_t i2c_conf = {
      .mode             = I2C_MODE_MASTER,
      .sda_io_num       = EXAMPLE_PIN_NUM_TOUCH_SDA,
      .sda_pullup_en    = GPIO_PULLUP_ENABLE,
      .scl_io_num       = EXAMPLE_PIN_NUM_TOUCH_SCL,
      .scl_pullup_en    = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 200 * 1000,
    };
    ESP_ERROR_CHECK(i2c_param_config(TOUCH_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0));

    esp_lcd_panel_io_handle_t           tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    // Attach the TOUCH to the I2C bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_HOST, &tp_io_config, &tp_io_handle));

    const esp_lcd_touch_config_t tp_cfg = {
      .x_max        = h_res - 1,
      .y_max        = v_res - 1,
      .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST,
      .int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT,
      .levels =
        {
          .reset     = 0,
          .interrupt = 0,
        },
      .flags =
        {
          .swap_xy  = 0,
          .mirror_x = 0,
          .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));
#endif

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
#endif
}