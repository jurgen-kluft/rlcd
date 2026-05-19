#include <Arduino.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_dev.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_compiler.h"

#include "ccore/c_memory.h"
#include "lib_wcs/c_lcd.h"
#include "lib_wcs/c_xl9555.h"
#include "lib_wcs/c_gt9xxx.h"

// -------------------------------------------------------------------------------------------------
// ------ ST7796 LCD PANEL COMMANDS ----------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

#define st7796_CMD_RAMCTRL            0xb5
#define st7796_DATA_LITTLE_ENDIAN_BIT (1 << 3)

static const char *LCD_PANEL_TAG = "lcd_panel.st7796";

static esp_err_t panel_st7796_del(esp_lcd_panel_t *panel);
static esp_err_t panel_st7796_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_st7796_init(esp_lcd_panel_t *panel);
static esp_err_t panel_st7796_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_st7796_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_st7796_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_st7796_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_st7796_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t panel_st7796_disp_on_off(esp_lcd_panel_t *panel, bool off);
static esp_err_t panel_st7796_sleep(esp_lcd_panel_t *panel, bool sleep);

struct st7796_panel_t
{
    esp_lcd_panel_t           base;
    esp_lcd_panel_io_handle_t io;
    int                       reset_gpio_num;
    bool                      reset_level;
    int                       x_gap;
    int                       y_gap;
    uint8_t                   fb_bits_per_pixel;
    uint8_t                   madctl_val;  // save current value of LCD_CMD_MADCTL register  保存LCD_CMD_MADCTL寄存器的当前值
    uint8_t                   colmod_val;  // save current value of LCD_CMD_COLMOD register  保存LCD_CMD_COLMOD寄存器的当前值
    uint8_t                   ramctl_val_1;
    uint8_t                   ramctl_val_2;
};

esp_err_t esp_lcd_new_panel_st7796(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    uint8_t fb_bits_per_pixel = 0;

#if CONFIG_LCD_ENABLE_DEBUG_LOG
    esp_log_level_set(LCD_PANEL_TAG, ESP_LOG_DEBUG);
#endif
    esp_err_t       ret    = ESP_OK;
    st7796_panel_t *st7796 = NULL;
    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, LCD_PANEL_TAG, "invalid argument");
    // leak detection of st7796 because saving st7796->base address  st7796的泄漏检测，因为保存st7796->基址
    ESP_COMPILER_DIAGNOSTIC_PUSH_IGNORE("-Wanalyzer-malloc-leak")
    st7796 = (st7796_panel_t *)calloc(1, sizeof(st7796_panel_t));
    ESP_GOTO_ON_FALSE(st7796, ESP_ERR_NO_MEM, err, LCD_PANEL_TAG, "no mem for st7796 panel");

    if (panel_dev_config->reset_gpio_num >= 0)
    {
        gpio_config_t io_conf;
        ncore::g_memclr(&io_conf, sizeof(gpio_config_t));  // Clear config structure
        io_conf.mode         = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num;

        ESP_GOTO_ON_ERROR(gpio_config(&io_conf), err, LCD_PANEL_TAG, "configure GPIO for RST line failed");
    }

    switch (panel_dev_config->rgb_ele_order)
    {
        case LCD_RGB_ELEMENT_ORDER_RGB: st7796->madctl_val = 0; break;
        case LCD_RGB_ELEMENT_ORDER_BGR: st7796->madctl_val |= LCD_CMD_BGR_BIT; break;
        default:
            // ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, LCD_PANEL_TAG, "unsupported RGB element order");
            goto err;
            break;
    }

    switch (panel_dev_config->bits_per_pixel)
    {
        case 16:  // RGB565
            st7796->colmod_val = 0x55;
            fb_bits_per_pixel  = 16;
            break;
        case 18:  // RGB666
            st7796->colmod_val = 0x66;
            // each color component (R/G/B) should occupy the 6 high bits of a byte, which means 3 full bytes are required for a pixel
            // 每个颜色组件（R/G/B）应该占据一个字节的6个高位，这意味着一个像素需要3个完整的字节
            fb_bits_per_pixel = 24;
            break;
        default: ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, LCD_PANEL_TAG, "unsupported pixel width"); break;
    }

    st7796->ramctl_val_1 = 0x00;
    st7796->ramctl_val_2 = 0xf0;  // Use big endian by default  默认使用大端序
    if ((panel_dev_config->data_endian) == LCD_RGB_DATA_ENDIAN_LITTLE)
    {
        // Use little endian  使用小尾端
        st7796->ramctl_val_2 |= st7796_DATA_LITTLE_ENDIAN_BIT;
    }

    st7796->io                = io;
    st7796->fb_bits_per_pixel = fb_bits_per_pixel;
    st7796->reset_gpio_num    = panel_dev_config->reset_gpio_num;
    st7796->reset_level       = panel_dev_config->flags.reset_active_high;
    st7796->base.del          = panel_st7796_del;
    st7796->base.reset        = panel_st7796_reset;
    st7796->base.init         = panel_st7796_init;
    st7796->base.draw_bitmap  = panel_st7796_draw_bitmap;
    st7796->base.invert_color = panel_st7796_invert_color;
    st7796->base.set_gap      = panel_st7796_set_gap;
    st7796->base.mirror       = panel_st7796_mirror;
    st7796->base.swap_xy      = panel_st7796_swap_xy;
    st7796->base.disp_on_off  = panel_st7796_disp_on_off;
    st7796->base.disp_sleep   = panel_st7796_sleep;
    *ret_panel                = &(st7796->base);
    ESP_LOGD(LCD_PANEL_TAG, "new st7796 panel @%p", st7796);

    return ESP_OK;

err:
    if (st7796)
    {
        if (panel_dev_config->reset_gpio_num >= 0)
        {
            gpio_reset_pin((gpio_num_t)panel_dev_config->reset_gpio_num);
        }
        free(st7796);
    }
    return ret;
    ESP_COMPILER_DIAGNOSTIC_POP("-Wanalyzer-malloc-leak")
}

static esp_err_t panel_st7796_del(esp_lcd_panel_t *panel)
{
    st7796_panel_t *st7796 = __containerof(panel, st7796_panel_t, base);

    if (st7796->reset_gpio_num >= 0)
    {
        gpio_reset_pin((gpio_num_t)st7796->reset_gpio_num);
    }
    ESP_LOGD(LCD_PANEL_TAG, "del st7796 panel @%p", st7796);
    free(st7796);
    return ESP_OK;
}

static esp_err_t panel_st7796_reset(esp_lcd_panel_t *panel)
{
    st7796_panel_t           *st7796 = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io     = st7796->io;

    // perform hardware reset  执行硬件复位
    if (st7796->reset_gpio_num >= 0)
    {
        gpio_set_level((gpio_num_t)st7796->reset_gpio_num, st7796->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)st7796->reset_gpio_num, !st7796->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    else
    {  // perform software reset  执行软件复位
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0), LCD_PANEL_TAG, "io tx param failed");
        vTaskDelay(pdMS_TO_TICKS(20));  // spec, wait at least 5m before sending new command  Spec，至少等待5m后再发送新命令
    }

    return ESP_OK;
}

static esp_err_t panel_st7796_init(esp_lcd_panel_t *panel)
{
    st7796_panel_t           *st7796 = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io     = st7796->io;
    // LCD goes into sleep mode and display will be turned off after power on reset, exit sleep mode first
    // LCD进入休眠模式，开机复位后显示屏关闭，请先退出休眠模式
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0), LCD_PANEL_TAG, "io tx param failed");
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL,
                                                  (uint8_t[]){
                                                    st7796->madctl_val,
                                                  },
                                                  1),
                        LCD_PANEL_TAG, "io tx param failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_COLMOD,
                                                  (uint8_t[]){
                                                    st7796->colmod_val,
                                                  },
                                                  1),
                        LCD_PANEL_TAG, "io tx param failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, st7796_CMD_RAMCTRL, (uint8_t[]){st7796->ramctl_val_1, st7796->ramctl_val_2}, 2), LCD_PANEL_TAG, "io tx param failed");

    return ESP_OK;
}

static esp_err_t panel_st7796_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    st7796_panel_t           *st7796 = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io     = st7796->io;

    x_start += st7796->x_gap;
    x_end += st7796->x_gap;
    y_start += st7796->y_gap;
    y_end += st7796->y_gap;

    // define an area of frame memory where MCU can access  定义一个区域的帧存储器，MCU可以访问
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET,
                                                  (uint8_t[]){
                                                    (uint8_t)((x_start >> 8) & 0xFF),
                                                    (uint8_t)(x_start & 0xFF),
                                                    (uint8_t)(((x_end - 1) >> 8) & 0xFF),
                                                    (uint8_t)((x_end - 1) & 0xFF),
                                                  },
                                                  4),
                        LCD_PANEL_TAG, "io tx param failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET,
                                                  (uint8_t[]){
                                                    (uint8_t)((y_start >> 8) & 0xFF),
                                                    (uint8_t)(y_start & 0xFF),
                                                    (uint8_t)(((y_end - 1) >> 8) & 0xFF),
                                                    (uint8_t)((y_end - 1) & 0xFF),
                                                  },
                                                  4),
                        LCD_PANEL_TAG, "io tx param failed");
    // transfer frame buffer  传输帧缓冲器
    size_t len = (x_end - x_start) * (y_end - y_start) * st7796->fb_bits_per_pixel / 8;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, color_data, len), LCD_PANEL_TAG, "io tx color failed");

    return ESP_OK;
}

static esp_err_t panel_st7796_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    st7796_panel_t           *st7796  = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io      = st7796->io;
    int                       command = 0;
    if (invert_color_data)
    {
        command = LCD_CMD_INVON;
    }
    else
    {
        command = LCD_CMD_INVOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), LCD_PANEL_TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_st7796_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    st7796_panel_t           *st7796 = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io     = st7796->io;
    if (mirror_x)
    {
        st7796->madctl_val |= LCD_CMD_MX_BIT;
    }
    else
    {
        st7796->madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y)
    {
        st7796->madctl_val |= LCD_CMD_MY_BIT;
    }
    else
    {
        st7796->madctl_val &= ~LCD_CMD_MY_BIT;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]){st7796->madctl_val}, 1), LCD_PANEL_TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_st7796_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    st7796_panel_t           *st7796 = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io     = st7796->io;
    if (swap_axes)
    {
        st7796->madctl_val |= LCD_CMD_MV_BIT;
    }
    else
    {
        st7796->madctl_val &= ~LCD_CMD_MV_BIT;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, (uint8_t[]){st7796->madctl_val}, 1), LCD_PANEL_TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_st7796_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    st7796_panel_t *st7796 = __containerof(panel, st7796_panel_t, base);
    st7796->x_gap          = x_gap;
    st7796->y_gap          = y_gap;
    return ESP_OK;
}

static esp_err_t panel_st7796_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    st7796_panel_t           *st7796  = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io      = st7796->io;
    int                       command = 0;
    if (on_off)
    {
        command = LCD_CMD_DISPON;
    }
    else
    {
        command = LCD_CMD_DISPOFF;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), LCD_PANEL_TAG, "io tx param failed");
    return ESP_OK;
}

static esp_err_t panel_st7796_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    st7796_panel_t           *st7796  = __containerof(panel, st7796_panel_t, base);
    esp_lcd_panel_io_handle_t io      = st7796->io;
    int                       command = 0;
    if (sleep)
    {
        command = LCD_CMD_SLPIN;
    }
    else
    {
        command = LCD_CMD_SLPOUT;
    }
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, command, NULL, 0), LCD_PANEL_TAG, "io tx param failed");
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// RGB_BL
#define LCD_BL(x)                                                                                                              \
    do                                                                                                                         \
    {                                                                                                                          \
        x ? ncore::nxl9555::pin_write(ncore::nxl9555::BL_CTR_IO, 1) : ncore::nxl9555::pin_write(ncore::nxl9555::BL_CTR_IO, 0); \
    } while (0)

// RGB_RST
#define LCD_RST(x)                                                                                                               \
    do                                                                                                                           \
    {                                                                                                                            \
        x ? ncore::nxl9555::pin_write(ncore::nxl9555::LCD_RST_IO, 1) : ncore::nxl9555::pin_write(ncore::nxl9555::LCD_RST_IO, 0); \
    } while (0)

/* 引脚定义 */
#define LCD_NUM_CS  GPIO_NUM_39
#define LCD_NUM_DC  GPIO_NUM_38
#define LCD_NUM_RD  GPIO_NUM_14
#define LCD_NUM_WR  GPIO_NUM_45
#define LCD_NUM_RST GPIO_NUM_NC

#define GPIO_LCD_D0  GPIO_NUM_13
#define GPIO_LCD_D1  GPIO_NUM_12
#define GPIO_LCD_D2  GPIO_NUM_11
#define GPIO_LCD_D3  GPIO_NUM_10
#define GPIO_LCD_D4  GPIO_NUM_9
#define GPIO_LCD_D5  GPIO_NUM_46
#define GPIO_LCD_D6  GPIO_NUM_3
#define GPIO_LCD_D7  GPIO_NUM_8
#define GPIO_LCD_D8  GPIO_NUM_18
#define GPIO_LCD_D9  GPIO_NUM_17
#define GPIO_LCD_D10 GPIO_NUM_16
#define GPIO_LCD_D11 GPIO_NUM_15
#define GPIO_LCD_D12 GPIO_NUM_7
#define GPIO_LCD_D13 GPIO_NUM_6
#define GPIO_LCD_D14 GPIO_NUM_5
#define GPIO_LCD_D15 GPIO_NUM_4

#define WHITE   0xFFFF
#define BLACK   0x0000
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define MAGENTA 0XF81F
#define YELLOW  0XFFE0
#define CYAN    0X07FF

#define BROWN      0XBC40
#define BRRED      0XFC07
#define GRAY       0X8430
#define DARKBLUE   0X01CF
#define LIGHTBLUE  0X7D7C
#define GRAYBLUE   0X5458
#define LIGHTGREEN 0X841F
#define LGRAY      0XC618
#define LGRAYBLUE  0XA651
#define LBBLUE     0X2B12

typedef struct _lcd_obj_t
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
} lcd_obj_t;

typedef struct _lcd_config_t
{
    void                                  *user_ctx;           /* 回调函数传入参数 */
    esp_lcd_panel_io_color_trans_done_cb_t notify_flush_ready; /* 刷新回调函数 */
} lcd_cfg_t;

static bool lcd_init(lcd_cfg_t lcd_config);
static void lcd_clear(uint16_t color);
static void lcd_display_dir(uint8_t dir);
static void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
static void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);

static const char     *LCD_TAG      = "LCD";
esp_lcd_panel_handle_t panel_handle = NULL; /* LCD句柄 */
uint32_t               g_back_color = 0xFFFF;
lcd_obj_t              lcd_dev;

static void lcd_clear(uint16_t color)
{
    uint16_t       y               = 0;
    const uint32_t MAX_BUFFER_SIZE = 65536;
    uint16_t       block_height    = MAX_BUFFER_SIZE / (lcd_dev.width * sizeof(uint16_t));
    if (block_height == 0)
        block_height = 1;

    uint16_t *buffer = (uint16_t *)heap_caps_malloc(lcd_dev.width * block_height * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (NULL == buffer)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }

    for (uint32_t i = 0; i < lcd_dev.width * block_height; i++)
    {
        buffer[i] = color;
    }

    while (y < lcd_dev.height)
    {
        uint16_t current_height = (y + block_height > lcd_dev.height) ? (lcd_dev.height - y) : block_height;

        esp_lcd_panel_draw_bitmap(panel_handle, 0, y, lcd_dev.width, y + current_height, buffer);

        y += current_height;
    }

    heap_caps_free(buffer);
    vTaskDelay(1);
}

static void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);

static void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    if (sx >= lcd_dev.width || sy >= lcd_dev.height || ex >= lcd_dev.width || ey >= lcd_dev.height || sx > ex || sy > ey)
    {
        ESP_LOGE(LCD_TAG, "Invalid fill area: sx=%d, sy=%d, ex=%d, ey=%d", sx, sy, ex, ey);
        return;
    }

    uint16_t width  = ex - sx + 1;
    uint16_t height = ey - sy + 1;

    uint16_t *buffer = (uint16_t *)heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_INTERNAL);
    if (NULL == buffer)
    {
        ESP_LOGE(LCD_TAG, "Memory for bitmap is not enough");
        return;
    }

    for (uint16_t i = 0; i < width; i++)
    {
        buffer[i] = color;
    }

    for (uint16_t y = 0; y < height; y++)
    {
        esp_lcd_panel_draw_bitmap(panel_handle, sx, sy + y, ex + 1, sy + y + 1, buffer);
    }

    lcd_draw_point(ex, ey, color);

    heap_caps_free(buffer);
}

static void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    if (sx >= lcd_dev.width || sy >= lcd_dev.height || ex >= lcd_dev.width || ey >= lcd_dev.height || sx > ex || sy > ey)
    {
        ESP_LOGE("LCD_TAG", "Invalid fill area: sx=%d, sy=%d, ex=%d, ey=%d", sx, sy, ex, ey);
        return;
    }

    uint16_t width        = ex - sx + 1;
    uint16_t height       = ey - sy + 1;
    uint32_t total_pixels = width * height;
    uint32_t buf_index    = 0;

    uint16_t *buffer = (uint16_t *)heap_caps_malloc(width * sizeof(uint16_t), MALLOC_CAP_INTERNAL);
    if (NULL == buffer)
    {
        ESP_LOGE(LCD_TAG, "Memory for bitmap is not enough");
        return;
    }

    for (uint16_t y_index = 0; y_index < height; y_index++)
    {
        /* 填充当前行的颜色数据 */
        for (uint16_t x_index = 0; x_index < width; x_index++)
        {
            if (buf_index < total_pixels)
            {
                buffer[x_index] = color[buf_index];
                buf_index++;
            }
        }

        esp_lcd_panel_draw_bitmap(panel_handle, sx, sy + y_index, ex + 1, sy + y_index + 1, buffer);
    }

    lcd_draw_point(ex, ey, color[total_pixels - 1]);

    heap_caps_free(buffer);
}

static void lcd_display_dir(uint8_t dir)
{
    lcd_dev.dir = dir;

    if (lcd_dev.dir == 0)
    {
        lcd_dev.width  = lcd_dev.pwidth;
        lcd_dev.height = lcd_dev.pheight;
        esp_lcd_panel_swap_xy(panel_handle, false);      /* 交换X和Y轴 */
        esp_lcd_panel_mirror(panel_handle, true, false); /* 对屏幕的Y轴不进行镜像处理 */
    }
    else if (lcd_dev.dir == 1)
    {
        lcd_dev.width  = lcd_dev.pheight;
        lcd_dev.height = lcd_dev.pwidth;
        esp_lcd_panel_swap_xy(panel_handle, true);        /* 不需要交换X和Y轴 */
        esp_lcd_panel_mirror(panel_handle, false, false); /* 对屏幕的XY轴进行镜像处理 */
    }
}

static void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color) { esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, (uint16_t *)&color); }

static bool lcd_init(lcd_cfg_t lcd_config)
{
    gpio_config_t gpio_init_struct;
    ncore::g_memclr(&gpio_init_struct, sizeof(gpio_config_t));

    esp_lcd_panel_io_handle_t io_handle = NULL;

    if (!ncore::nxl9555::init())
    {
        ESP_LOGE(LCD_TAG, "Failed to initialize I/O expander");
        return false;
    }

    lcd_dev.wr = LCD_NUM_WR; /* 配置WR引脚 */
    lcd_dev.cs = LCD_NUM_CS; /* 配置CS引脚 */
    lcd_dev.dc = LCD_NUM_DC; /* 配置DC引脚 */
    lcd_dev.rd = LCD_NUM_RD; /* 配置RD引脚 */

    lcd_dev.pwidth  = 320; /* 面板宽度,单位:像素 */
    lcd_dev.pheight = 480; /* 面板高度,单位:像素 */

    /* 配置RD引脚 */
    gpio_init_struct.intr_type    = GPIO_INTR_DISABLE;      /* 失能引脚中断 */
    gpio_init_struct.mode         = GPIO_MODE_INPUT_OUTPUT; /* 配置输出模式 */
    gpio_init_struct.pin_bit_mask = 1ull << lcd_dev.rd;     /* 配置引脚位掩码 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;  /* 失能下拉 */
    gpio_init_struct.pull_up_en   = GPIO_PULLUP_ENABLE;     /* 使能下拉 */
    gpio_config(&gpio_init_struct);                         /* 引脚配置 */
    gpio_set_level((gpio_num_t)lcd_dev.rd, 1);              /* RD管脚拉高 */

    esp_lcd_i80_bus_handle_t i80_bus = NULL;

    esp_lcd_i80_bus_config_t bus_config;
    ncore::g_memclr(&bus_config, sizeof(esp_lcd_i80_bus_config_t));
    bus_config.clk_src            = LCD_CLK_SRC_DEFAULT;
    bus_config.dc_gpio_num        = lcd_dev.dc;
    bus_config.wr_gpio_num        = lcd_dev.wr;
    bus_config.data_gpio_nums[0]  = GPIO_LCD_D0;
    bus_config.data_gpio_nums[1]  = GPIO_LCD_D1;
    bus_config.data_gpio_nums[2]  = GPIO_LCD_D2;
    bus_config.data_gpio_nums[3]  = GPIO_LCD_D3;
    bus_config.data_gpio_nums[4]  = GPIO_LCD_D4;
    bus_config.data_gpio_nums[5]  = GPIO_LCD_D5;
    bus_config.data_gpio_nums[6]  = GPIO_LCD_D6;
    bus_config.data_gpio_nums[7]  = GPIO_LCD_D7;
    bus_config.data_gpio_nums[8]  = GPIO_LCD_D8;
    bus_config.data_gpio_nums[9]  = GPIO_LCD_D9;
    bus_config.data_gpio_nums[10] = GPIO_LCD_D10;
    bus_config.data_gpio_nums[11] = GPIO_LCD_D11;
    bus_config.data_gpio_nums[12] = GPIO_LCD_D12;
    bus_config.data_gpio_nums[13] = GPIO_LCD_D13;
    bus_config.data_gpio_nums[14] = GPIO_LCD_D14;
    bus_config.data_gpio_nums[15] = GPIO_LCD_D15;
    bus_config.bus_width          = 16;
    bus_config.max_transfer_bytes = lcd_dev.pwidth * lcd_dev.pheight * sizeof(uint16_t);
    bus_config.psram_trans_align  = 64;
    bus_config.sram_trans_align   = 4;

    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus)); /* 新建80并口总线 */

    esp_lcd_panel_io_i80_config_t io_config;
    ncore::g_memclr(&io_config, sizeof(esp_lcd_panel_io_i80_config_t));
    io_config.cs_gpio_num              = lcd_dev.cs;
    io_config.pclk_hz                  = 40 * 1000 * 1000;
    io_config.trans_queue_depth        = 32;
    io_config.dc_levels.dc_idle_level  = 0;
    io_config.dc_levels.dc_cmd_level   = 0;
    io_config.dc_levels.dc_dummy_level = 0;
    io_config.dc_levels.dc_data_level  = 1;
    io_config.flags.swap_color_bytes   = 0;
    io_config.on_color_trans_done      = lcd_config.notify_flush_ready;
    io_config.user_ctx                 = lcd_config.user_ctx;
    io_config.lcd_cmd_bits             = 8;
    io_config.lcd_param_bits           = 8;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    LCD_RST(1);
    vTaskDelay(10);
    LCD_RST(0);
    vTaskDelay(50);
    LCD_RST(1);
    vTaskDelay(200);

    // esp_lcd_panel_dev_config_t panel_config = {
    //   .reset_gpio_num = LCD_NUM_RST,
    //   .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_BGR,
    //   .bits_per_pixel = 16,
    // };
    esp_lcd_panel_dev_config_t panel_config;
    ncore::g_memclr(&panel_config, sizeof(esp_lcd_panel_dev_config_t));
    panel_config.reset_gpio_num = LCD_NUM_RST;
    panel_config.rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_BGR;
    panel_config.bits_per_pixel = 16;

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));

    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_invert_color(panel_handle, true);
    esp_lcd_panel_set_gap(panel_handle, 0, 0);
    esp_lcd_panel_io_tx_param(io_handle, 0x36, (uint8_t[]){0x08}, 1);
    esp_lcd_panel_io_tx_param(io_handle, 0x3A, (uint8_t[]){0x55}, 1);

    lcd_display_dir(0);
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    lcd_clear(WHITE);
    LCD_BL(1);

    return true;
}

#include "lib_wcs/c_lcd.h"

namespace ncore
{
    namespace nlcd
    {
        bool initialize()
        {
            if (!nxl9555::init())
                return false;

            // initialize LCD
            lcd_cfg_t lcd_config = {
              .user_ctx           = NULL,
              .notify_flush_ready = NULL,
            };

            return lcd_init(lcd_config);
        }
        u16 width() { return lcd_dev.width; }
        u16 height() { return lcd_dev.height; }

        void rotation(u8 rotation)
        {
            switch (rotation & 3)
            {
                case 0: lcd_display_dir(0); break;
                case 1: lcd_display_dir(1); break;
                case 2: lcd_display_dir(0); break;
                case 3: lcd_display_dir(1); break;
            }
        }

        void orientation(u8 orientation)
        {
            // orientation: 0-3, 0: normal, 1: mirror x, 2: mirror y, 3: mirror xy
            switch (orientation & 3)
            {
                case 0: esp_lcd_panel_mirror(panel_handle, false, false); break;
                case 1: esp_lcd_panel_mirror(panel_handle, true, false); break;
                case 2: esp_lcd_panel_mirror(panel_handle, false, true); break;
                case 3: esp_lcd_panel_mirror(panel_handle, true, true); break;
            }
        }

        void draw_rectangle(u16 sx, u16 sy, u16 ex, u16 ey, u16 color) { lcd_fill(sx, sy, ex, ey, color); }
        void draw_sprite(u16 sx, u16 sy, u16 ex, u16 ey, const u16 *color)
        {
            if (sx >= lcd_dev.width || sy >= lcd_dev.height || ex >= lcd_dev.width || ey >= lcd_dev.height || sx > ex || sy > ey)
            {
                ESP_LOGE("LCD_TAG", "Invalid sprite area: sx=%d, sy=%d, ex=%d, ey=%d", sx, sy, ex, ey);
                return;
            }

            uint16_t width  = ex - sx + 1;
            uint16_t height = ey - sy + 1;

            esp_lcd_panel_draw_bitmap(panel_handle, sx, sy, ex + 1, ey + 1, color);
        }

        void led_switch(bool on)
        {
            if (on)
                nxl9555::pin_write(ncore::nxl9555::LED1_IO, 1);
            else
                nxl9555::pin_write(ncore::nxl9555::LED1_IO, 0);
        }
        void led_toggle() { nxl9555::pin_toggle(ncore::nxl9555::LED1_IO); }

        void backlight_switch(bool on) { LCD_BL(on ? 1 : 0); }

    }  // namespace nlcd
}  // namespace ncore

// uint64_t toggle_led_time      = 0;
// uint64_t toggle_lcd_fill_time = 0;

// void loop()
// {
//     // Example: Toggle LED1 every second
//     if (millis() - toggle_led_time > 1000)
//     {
//         LED1_TOGGLE();
//         toggle_led_time = millis();
//     }

//     if (millis() - toggle_lcd_fill_time > 2000)
//     {
//         // Fill a random area with a random color every 2 seconds
//         uint16_t color = random(0xFFFF);  // Random RGB565 color
//         uint16_t sx    = random(lcd_dev.width);
//         uint16_t sy    = random(lcd_dev.height);
//         uint16_t ex    = sx + random(10, 50);  // Random width between 10 and 50 pixels
//         uint16_t ey    = sy + random(10, 50);  // Random height between 10 and 50 pixels

//         // Ensure ex and ey are within bounds
//         if (ex >= lcd_dev.width)
//             ex = lcd_dev.width - 1;
//         if (ey >= lcd_dev.height)
//             ey = lcd_dev.height - 1;

//         lcd_fill(sx, sy, ex, ey, color);
//         toggle_lcd_fill_time = millis();
//     }
// }
