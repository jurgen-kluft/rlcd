#include <stdlib.h>
#include <sys/cdefs.h>

#define EXAMPLE_USE_TOUCH 0

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"

#if EXAMPLE_USE_TOUCH
    #include "driver/i2c.h"
    #define TOUCH_HOST I2C_NUM_0
#endif

#include "ccore/c_memory.h"
#include "rlcd/private/esp_lcd_sh8601.h"
#include "rlcd/private/esp_lcd_touch_ft5x06.h"
#include "rlcd/c_dwo_amoled.h"

static const char *TAG = "rlcd";

#define LCD_HOST SPI2_HOST

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

#if EXAMPLE_USE_TOUCH
    #define EXAMPLE_PIN_NUM_TOUCH_SCL (GPIO_NUM_48)
    #define EXAMPLE_PIN_NUM_TOUCH_SDA (GPIO_NUM_47)
    #define EXAMPLE_PIN_NUM_TOUCH_RST (GPIO_NUM_3)
    #define EXAMPLE_PIN_NUM_TOUCH_INT (-1)

esp_lcd_touch_handle_t tp = NULL;
#else
void *tp = NULL;

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

namespace ndwo
{
    static bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
    {
        user_notify_flush_ready_t *user_notify_flush = (user_notify_flush_ready_t *)user_ctx;
        if (user_notify_flush && user_notify_flush->m_cb)
        {
            user_notify_flush->m_cb(user_notify_flush->m_user_ctx);
        }
        return false;
    }

    bool init_dwo_display(int32_t h_res, int32_t v_res, int8_t bpp, user_notify_flush_ready_t *user_notify_flush_ready, panel_handles_t *out_handles)
    {
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
        ESP_LOGI(TAG, "Turn off LCD backlight");
        gpio_config_t bk_gpio_config = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
        ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif

        ESP_LOGI(TAG, "Initialize SPI bus");

        spi_bus_config_t buscfg;
        ncore::g_memset(&buscfg, 0, sizeof(spi_bus_config_t));
        buscfg.sclk_io_num     = EXAMPLE_PIN_NUM_LCD_PCLK;
        buscfg.data0_io_num    = EXAMPLE_PIN_NUM_LCD_DATA0;
        buscfg.data1_io_num    = EXAMPLE_PIN_NUM_LCD_DATA1;
        buscfg.data2_io_num    = EXAMPLE_PIN_NUM_LCD_DATA2;
        buscfg.data3_io_num    = EXAMPLE_PIN_NUM_LCD_DATA3;
        buscfg.max_transfer_sz = h_res * v_res * bpp / 8;
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

        ESP_LOGI(TAG, "Install panel IO");
        esp_lcd_panel_io_handle_t     io_handle = NULL;
        esp_lcd_panel_io_spi_config_t io_config;
        ncore::g_memset(&io_config, 0, sizeof(esp_lcd_panel_io_spi_config_t));
        io_config.cs_gpio_num         = EXAMPLE_PIN_NUM_LCD_CS;
        io_config.dc_gpio_num         = -1;
        io_config.spi_mode            = 0;
        io_config.pclk_hz             = 40 * 1000 * 1000;
        io_config.trans_queue_depth   = 10;
        io_config.on_color_trans_done = notify_flush_ready;
        io_config.user_ctx            = user_notify_flush_ready;
        io_config.lcd_cmd_bits        = 32;
        io_config.lcd_param_bits      = 8;
        io_config.flags.quad_mode     = true;

        sh8601_vendor_config_t vendor_config;
        ncore::g_memset(&vendor_config, 0, sizeof(sh8601_vendor_config_t));
        vendor_config.init_cmds                = lcd_init_cmds;
        vendor_config.init_cmds_size           = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]);
        vendor_config.flags.use_qspi_interface = 1;

        // Attach the LCD to the SPI bus
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

        esp_lcd_panel_handle_t     panel_handle = NULL;
        esp_lcd_panel_dev_config_t panel_config;
        ncore::g_memset(&panel_config, 0, sizeof(esp_lcd_panel_dev_config_t));
        panel_config.reset_gpio_num          = EXAMPLE_PIN_NUM_LCD_RST;
        panel_config.rgb_ele_order           = LCD_RGB_ELEMENT_ORDER_RGB;
        panel_config.bits_per_pixel          = bpp;
        panel_config.flags.reset_active_high = 0;
        panel_config.vendor_config           = &vendor_config;

        ESP_LOGI(TAG, "Install SH8601 panel driver");
        ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
        // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if EXAMPLE_USE_TOUCH
        ESP_LOGI(TAG, "Initialize I2C bus");
        i2c_config_t i2c_conf;
        ncore::g_memset(&i2c_conf, 0, sizeof(i2c_config_t));
        i2c_conf.mode             = I2C_MODE_MASTER;
        i2c_conf.sda_io_num       = EXAMPLE_PIN_NUM_TOUCH_SDA;
        i2c_conf.sda_pullup_en    = GPIO_PULLUP_ENABLE;
        i2c_conf.scl_io_num       = EXAMPLE_PIN_NUM_TOUCH_SCL;
        i2c_conf.scl_pullup_en    = GPIO_PULLUP_ENABLE;
        i2c_conf.master.clk_speed = 200 * 1000;

        ESP_ERROR_CHECK(i2c_param_config(TOUCH_HOST, &i2c_conf));
        ESP_ERROR_CHECK(i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0));

        esp_lcd_panel_io_i2c_config_t tp_io_config;
        ncore::g_memset(&tp_io_config, 0, sizeof(esp_lcd_panel_io_i2c_config_t));
        tp_io_config.dev_addr                    = ESP_LCD_TOUCH_IO_I2C_FT5x06_ADDRESS;
        tp_io_config.control_phase_bytes         = 1;
        tp_io_config.dc_bit_offset               = 0;
        tp_io_config.lcd_cmd_bits                = 8;
        tp_io_config.flags.disable_control_phase = 1;

        // Attach the TOUCH to the I2C bus
        esp_lcd_panel_io_handle_t tp_io_handle = NULL;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_HOST, &tp_io_config, &tp_io_handle));

        esp_lcd_touch_config_t tp_cfg;
        ncore::g_memset(&tp_cfg, 0, sizeof(esp_lcd_touch_config_t));
        tp_cfg.x_max            = h_res - 1;
        tp_cfg.y_max            = v_res - 1;
        tp_cfg.rst_gpio_num     = (gpio_num_t)EXAMPLE_PIN_NUM_TOUCH_RST;
        tp_cfg.int_gpio_num     = (gpio_num_t)EXAMPLE_PIN_NUM_TOUCH_INT;
        tp_cfg.levels.reset     = 0;
        tp_cfg.levels.interrupt = 0;
        tp_cfg.flags.swap_xy    = 0;
        tp_cfg.flags.mirror_x   = 0;
        tp_cfg.flags.mirror_y   = 0;

        ESP_LOGI(TAG, "Initialize touch controller");
        ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));
#endif

#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
        ESP_LOGI(TAG, "Turn on LCD backlight");
        gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
#endif

        out_handles->panel_handle = (void *)panel_handle;
        out_handles->touch_handle = (void *)tp;

        return true;
    }

}  // namespace ndwo