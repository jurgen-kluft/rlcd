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
            tp.width  = width;
            tp.height = height;

            for (int i = 0; i < TP_CT_MAX_TOUCH; i++)
            {
                tp.points[i].x = 0;
                tp.points[i].y = 0;
            }
            tp.touchstatus = 0;

            tp.touchtype  = portrait ? 0 : 0X01;  // Set landscape/portrait
            tp.gt9xxx_dev = ngt9xxx::create();    // Initialize the GT9xxx device
            tp.touchtype |= 0X80;                 // Capacitive touchscreen

            tp.polling_interval = 0;  // Initialize polling interval counter
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

            ngt9xxx::device_t *dev     = tp.gt9xxx_dev;
            const u8           max_tps = ngt9xxx::max_tps(dev);

            tp.polling_interval++;
            if ((tp.polling_interval % 10) == 0 || tp.polling_interval < 10)  // When idle, check once every 10 scans to reduce CPU usage
            {
                ngt9xxx::read_reg(dev, ngt9xxx::GT9XXX_GSTID_REG, &mode, 1);  // Read touch-point status

                const u8 num_touch_points = mode & 0x0F;  // Extract number of touch points from status

                if ((mode & 0X80) && (num_touch_points <= max_tps))
                {
                    i = 0;
                    ngt9xxx::write_reg(dev, ngt9xxx::GT9XXX_GSTID_REG, &i, 1);  // Clear flag
                }

                if ((mode & 0XF) && (num_touch_points <= max_tps))
                {
                    const u16 num_touch_points_mask = 0XFFFF << num_touch_points;  // Convert number of points to a bitmask matching tp.touchstatus
                    tp.touchstatus                  = (~num_touch_points_mask) | cTP_PRES_DOWN | cTP_CATH_PRES;

                    for (i = 0; i < max_tps; i++)
                    {
                        if (tp.touchstatus & (1 << i))  // Touch valid?
                        {
                            ngt9xxx::read_tpx_reg(dev, i, buf, 4);  // Read XY coordinates

                            if (tp_is_portrait(tp))  // Portrait
                            {
                                tp.points[i].x = (buf[1] << 8) | buf[0];  // X coordinate
                                tp.points[i].y = (buf[3] << 8) | buf[2];  // Y coordinate
                            }
                            else  // Landscape: swap X and Y, and invert Y
                            {
                                tp.points[i].x = tp.width - (buf[3] << 8) | buf[2];  // Y coordinate
                                tp.points[i].y = (buf[1] << 8) | buf[0];             // X coordinate
                            }
                        }
                        else
                        {
                            tp.points[i].x = 0xffff;  // Invalid point: set to out-of-range value
                            tp.points[i].y = 0xffff;
                        }

                        // ESP_LOGI("GT9XXX", "%d, x=%d,y=%d\r\n", i, tp.points[i].x, tp.points[i].y);
                    }

                    res                 = 1;
                    tp.polling_interval = 0;  // After trigger, force at least 10 consecutive scans to improve hit rate
                }
            }

            if ((mode & 0X8F) == 0X80)  // No touch point pressed
            {
                if (tp.touchstatus & cTP_PRES_DOWN)  // Was previously pressed
                {
                    tp.touchstatus &= ~cTP_PRES_DOWN;  // Mark key release
                }
                else  // Was already not pressed
                {
                    tp.points[0].x = 0xffff;
                    tp.points[0].y = 0xffff;
                    tp.touchstatus &= 0XE000;  // Clear valid-point flags
                }
            }

            if (tp.polling_interval > 240)
            {
                tp.polling_interval = 10;  // Restart counting from 10
            }

            return res != 0;
        }

    }  // namespace ntouch
}  // namespace ncore
