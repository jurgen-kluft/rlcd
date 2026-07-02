#include "ccore/c_memory.h"
#include "rcore/c_log.h"

#include "lib_touch/c_touch.h"

namespace ncore
{
    namespace ntouch
    {
        void touch_point_transform(touch_point_t& tp, u16 width, u16 height, erotate_t rotation, emirror_t mirror)
        {
            u16 x         = tp.m_x;
            u16 y         = tp.m_y;
            u16 current_w = width;
            u16 current_h = height;

            // 1. Apply Rotation
            switch (rotation)
            {
                case cTP_ROTATE_90:
                    tp.m_x = current_h - 1 - y;
                    tp.m_y = x;
                    // Dimensions swap after 90 degree rotation
                    current_w = height;
                    current_h = width;
                    break;

                case cTP_ROTATE_180:
                    tp.m_x = current_w - 1 - x;
                    tp.m_y = current_h - 1 - y;
                    break;

                case cTP_ROTATE_270:
                    tp.m_x = y;
                    tp.m_y = current_w - 1 - x;
                    // Dimensions swap after 270 degree rotation
                    current_w = height;
                    current_h = width;
                    break;

                case cTP_ROTATE_0:
                default: break;
            }

            // 2. Apply Mirroring
            switch (mirror)
            {
                case cTP_MIRROR_X:  // Flip horizontally
                    tp.m_x = current_w - 1 - tp.m_x;
                    break;

                case cTP_MIRROR_Y:  // Flip vertically
                    tp.m_y = current_h - 1 - tp.m_y;
                    break;

                case cTP_MIRROR_XY:  // Flip both
                    tp.m_x = current_w - 1 - tp.m_x;
                    tp.m_y = current_h - 1 - tp.m_y;
                    break;

                case cTP_MIRROR_NONE:
                default: break;
            }
        }

        void touch_init(touch_t& tp, u16 width, u16 height, u64 polling_interval_ms, erotate_t rotation, emirror_t mirror)
        {
            tp.m_width               = 480;
            tp.m_height              = 480;
            tp.m_rotation            = (u8)rotation;
            tp.m_mirror              = (u8)mirror;
            tp.m_polling_interval    = polling_interval_ms;
            tp.m_touch_panel         = nullptr;
            tp.m_touch_panel_scan_fn = tp_scan;
        }

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   false: no touch
        //   true : touch detected
        bool touch_scan(touch_t& tp, u64 now, touch_point_t* points, u8* touches)
        {
            bool tp_scan_result = false;
            *touches            = 0;
            if (tp.m_touch_panel_scan_fn != NULL)
            {
                if (now > tp.m_polling_interval)
                {
                    tp.m_polling_interval = now + 20;                                             // Set next polling interval to 20ms later (50 Hz)
                    tp_scan_result        = tp.m_touch_panel_scan_fn(&tp, now, points, touches);  // Read touch data from the GT911 controller
                }
            }

            return tp_scan_result;
        }

    }  // namespace ntouch
}  // namespace ncore