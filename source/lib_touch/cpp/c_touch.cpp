#include "ccore/c_memory.h"
#include "rcore/c_log.h"

#include "lib_touch/c_touch.h"

namespace ncore
{
    namespace ntouch
    {
        void touch_point_transform(touch_point_t& tp, u16 width, u16 height, erotate_t rotation, emirror_t mirror)
        {
            const u16 x = tp.m_x;
            const u16 y = tp.m_y;
            u16       w = width;
            u16       h = height;

            // 1. Apply Rotation
            switch (rotation)
            {
                case cTP_ROTATE_90:
                    tp.m_x = h - 1 - y;
                    tp.m_y = x;
                    // Dimensions swap after 90 degree rotation
                    w = height;
                    h = width;
                    break;

                case cTP_ROTATE_180:
                    tp.m_x = w - 1 - x;
                    tp.m_y = h - 1 - y;
                    break;

                case cTP_ROTATE_270:
                    tp.m_x = y;
                    tp.m_y = w - 1 - x;
                    // Dimensions swap after 270 degree rotation
                    w = height;
                    h = width;
                    break;

                case cTP_ROTATE_0:
                default: break;
            }

            // 2. Apply Mirroring
            switch (mirror)
            {
                case cTP_MIRROR_X:  // Flip horizontally
                    tp.m_x = w - 1 - tp.m_x;
                    break;

                case cTP_MIRROR_Y:  // Flip vertically
                    tp.m_y = h - 1 - tp.m_y;
                    break;

                case cTP_MIRROR_XY:  // Flip both
                    tp.m_x = w - 1 - tp.m_x;
                    tp.m_y = h - 1 - tp.m_y;
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
            tp.m_polling_interval_ms = polling_interval_ms;
            tp.m_touch_panel         = nullptr;
            tp.m_touch_panel_scan_fn = nullptr;
        }

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   false: no scan available
        //   true : scan available, touches updated
        bool touch_scan(touch_t& tp, u64 now_ms, touch_point_t* points, u8 max_touches, u8* num_touches)
        {
            *num_touches = 0;

            bool tp_scan_result = false;
            if (tp.m_touch_panel_scan_fn != NULL)
            {
                if (now_ms > tp.m_polling_interval_ms)
                {
                    tp.m_polling_interval_ms = now_ms + 20;                                  // Set next polling interval to 20ms later (50 Hz)
                    tp.m_touch_panel_scan_fn(tp, points, max_touches, num_touches);  // Read touch data from the GT911 controller
                    tp_scan_result = true;
                }
            }

            return tp_scan_result;
        }

    }  // namespace ntouch
}  // namespace ncore