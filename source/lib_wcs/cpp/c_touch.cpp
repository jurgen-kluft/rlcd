#include "lib_wcs/c_touch.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "string.h"

#include "lib_wcs/c_touch.h"
#include "lib_wcs/c_gt9xxx.h"

namespace ncore
{
    namespace ntouch
    {
        bool tp_init(touch_panel_t &tp, u16 width, u16 height, bool portrait)
        {
            tp.m_width  = width;
            tp.m_height = height;

            for (int i = 0; i < TP_CT_MAX_TOUCH; i++)
            {
                tp.m_points[i].m_x = 0;
                tp.m_points[i].m_y = 0;
            }
            tp.m_touchstatus = 0;

            tp.m_touchtype = portrait ? 0 : 0X01;  // Set landscape/portrait
            ngt9xxx::init();                       // Initialize the GT9xxx device
            tp.m_touchtype |= 0X80;                // Capacitive touchscreen

            tp.m_polling_interval = 0;  // Initialize polling interval counter
            return true;
        }

        const u16 cTP_PRES_DOWN = 0x8000;  // Touchscreen is pressed bit
        const u16 cTP_CATH_PRES = 0x4000;  // Touch point is active bit

        // Scan touchscreen (polling mode)
        // mode: unused for capacitive touch; kept for resistive-screen compatibility
        // return: current touch status
        //   0: no touch
        //   1: touch detected
        bool tp_scan(touch_panel_t &tp, u8 mode)
        {
            u8 buf[4];
            u8 i   = 0;
            u8 res = 0;

            const u8 max_tps = ngt9xxx::max_tps();

            tp.m_polling_interval++;
            if ((tp.m_polling_interval % 10) == 0 || tp.m_polling_interval < 10)  // When idle, check once every 10 scans to reduce CPU usage
            {
                ngt9xxx::read_reg(ngt9xxx::GT9XXX_GSTID_REG, &mode, 1);  // Read touch-point status

                const u8 num_touch_points = mode & 0x0F;  // Extract number of touch points from status

                if ((mode & 0X80) && (num_touch_points <= max_tps))
                {
                    i = 0;
                    ngt9xxx::write_reg(ngt9xxx::GT9XXX_GSTID_REG, &i, 1);  // Clear flag
                }

                if ((mode & 0XF) && (num_touch_points <= max_tps))
                {
                    const u16 num_touch_points_mask = 0XFFFF << num_touch_points;  // Convert number of points to a bitmask matching tp.m_touchstatus
                    tp.m_touchstatus                = (~num_touch_points_mask) | cTP_PRES_DOWN | cTP_CATH_PRES;

                    for (i = 0; i < max_tps; i++)
                    {
                        if (tp.m_touchstatus & (1 << i))  // Touch valid?
                        {
                            ngt9xxx::read_tpx_reg(i, buf, 4);  // Read XY coordinates

                            if (tp_is_portrait(tp))  // Portrait
                            {
                                tp.m_points[i].m_x = (buf[1] << 8) | buf[0];  // X coordinate
                                tp.m_points[i].m_y = (buf[3] << 8) | buf[2];  // Y coordinate
                            }
                            else  // Landscape: swap X and Y, and invert Y
                            {
                                tp.m_points[i].m_x = tp.m_width - (buf[3] << 8) | buf[2];  // Y coordinate
                                tp.m_points[i].m_y = (buf[1] << 8) | buf[0];               // X coordinate
                            }
                        }
                        else
                        {
                            tp.m_points[i].m_x = 0xffff;  // Invalid point: set to out-of-range value
                            tp.m_points[i].m_y = 0xffff;
                        }

                        // ESP_LOGI("GT9XXX", "%d, x=%d,y=%d\r\n", i, tp.m_points[i].m_x, tp.m_points[i].m_y);
                    }

                    res                   = 1;
                    tp.m_polling_interval = 0;  // After trigger, force at least 10 consecutive scans to improve hit rate
                }
            }

            if ((mode & 0X8F) == 0X80)  // No touch point pressed
            {
                if (tp.m_touchstatus & cTP_PRES_DOWN)  // Was previously pressed
                {
                    tp.m_touchstatus &= ~cTP_PRES_DOWN;  // Mark key release
                }
                else  // Was already not pressed
                {
                    tp.m_points[0].m_x = 0xffff;
                    tp.m_points[0].m_y = 0xffff;
                    tp.m_touchstatus &= 0XE000;  // Clear valid-point flags
                }
            }

            if (tp.m_polling_interval > 240)
            {
                tp.m_polling_interval = 10;  // Restart counting from 10
            }

            return res != 0;
        }

    }  // namespace ntouch
}  // namespace ncore
