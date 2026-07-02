#include "Arduino.h"

#include "esp_lcd_touch/esp_lcd_touch.h"

#include "ccore/c_memory.h"
#include "rcore/c_log.h"
#include "lib_guition/c_touch.h"

extern esp_lcd_touch_handle_t g_touch_handle;

namespace ncore
{
    namespace ntouch
    {
        const u16 cTP_PRES_DOWN = 0x8000;  // Touchscreen is pressed bit
        const u16 cTP_CATH_PRES = 0x4000;  // Touch point is active bit

        bool tp_init(touch_panel_t& tp, u16 width, u16 height, bool portrait)
        {
            bool tp_init_result = false;

            tp.m_width            = 480;
            tp.m_height           = 480;
            tp.m_isLargeDetect    = false;
            tp.m_touches          = 0;
            tp.m_rotation         = ROTATION_NORMAL;
            tp.m_polling_interval = 0;
            tp.m_touch_status     = 0;

            g_memset(tp.m_points, 0, TP_CT_MAX_TOUCH * sizeof(touch_point_t));

            if (g_touch_handle != NULL)
            {
                if (portrait)
                {
                    esp_lcd_touch_set_mirror_x(g_touch_handle, false);
                    esp_lcd_touch_set_mirror_y(g_touch_handle, false);
                }
                else
                {
                    esp_lcd_touch_set_mirror_x(g_touch_handle, true);
                    esp_lcd_touch_set_mirror_y(g_touch_handle, true);
                }
                tp_init_result = true;
            }

            return tp_init_result;
        }

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   false: no touch
        //   true : touch detected
        bool tp_scan(touch_panel_t& tp, u64 now)
        {
            bool tp_scan_result = false;
            if (g_touch_handle != NULL)
            {
                if (now > tp.m_polling_interval)
                {
                    tp.m_polling_interval = now + 20;  // Set next polling interval to 20ms later (50 Hz)

                    if (esp_lcd_touch_read_data(g_touch_handle) == ESP_OK)  // Read touch data from the GT911 controller
                    {
                        uint16_t touch_x[TP_CT_MAX_TOUCH];
                        uint16_t touch_y[TP_CT_MAX_TOUCH];
                        uint16_t touch_strength[TP_CT_MAX_TOUCH];
                        uint8_t  touch_cnt = 0;

                        bool data_ready = esp_lcd_touch_get_coordinates(g_touch_handle, touch_x, touch_y, touch_strength, &touch_cnt, TP_CT_MAX_TOUCH);
                        // bool data_ready = false;

                        if (!data_ready || touch_cnt == 0)  // No touch point pressed
                        {
                            tp.m_touches = 0;
                            if (tp.m_touch_status & cTP_PRES_DOWN)  // Was previously pressed
                            {
                                tp.m_touch_status &= ~cTP_PRES_DOWN;  // Mark key release
                            }
                            else  // Was already not pressed
                            {
                                tp.m_points[0].m_id   = 0;
                                tp.m_points[0].m_size = 0;
                                tp.m_points[0].m_x    = 0xffff;
                                tp.m_points[0].m_y    = 0xffff;
                                tp.m_touch_status &= 0XE000;  // Clear valid-point flags
                            }
                        }
                        else
                        {
                            tp_scan_result = true;
                            tp.m_touches   = touch_cnt;

                            const u8  num_touch_points      = touch_cnt;
                            const u16 num_touch_points_mask = 0XFFFF << num_touch_points;  // Convert number of points to a bitmask matching tp.m_touch_status
                            tp.m_touch_status               = (~num_touch_points_mask) | cTP_PRES_DOWN | cTP_CATH_PRES;

                            for (u8 i = 0; i < TP_CT_MAX_TOUCH; i++)
                            {
                                if (tp.m_touch_status & (1 << i))  // Touch valid?
                                {
                                    tp.m_points[i].m_id   = i;
                                    tp.m_points[i].m_size = touch_strength[i];
                                    tp.m_points[i].m_x    = touch_x[i];
                                    tp.m_points[i].m_y    = touch_y[i];
                                }
                                else
                                {
                                    tp.m_points[i].m_id   = 0;
                                    tp.m_points[i].m_size = 0;
                                    tp.m_points[i].m_x    = 0xffff;  // Invalid point: set to out-of-range value
                                    tp.m_points[i].m_y    = 0xffff;
                                }
                            }
                        }
                    }
                    else
                    {
                        nlog::log_error("ntouch", "Failed to read touch data from GT911 controller");
                    }
                }
            }

            return tp_scan_result;
        }

    }  // namespace ntouch
}  // namespace ncore