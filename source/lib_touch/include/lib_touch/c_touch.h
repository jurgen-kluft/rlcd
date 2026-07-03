#ifndef __RLCD_LIBTOUCH_TOUCH_H__
#define __RLCD_LIBTOUCH_TOUCH_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace ntouch
    {
        // Number of points supported by capacitive touch, fixed at 5 points
        const u8 TP_CT_MAX_TOUCH = 5;

        enum erotate_t
        {
            cTP_ROTATE_0   = 0,
            cTP_ROTATE_90  = 1,
            cTP_ROTATE_180 = 2,
            cTP_ROTATE_270 = 3
        };

        enum emirror_t
        {
            cTP_MIRROR_NONE = 0,
            cTP_MIRROR_X    = 1,
            cTP_MIRROR_Y    = 2,
            cTP_MIRROR_XY   = 3
        };

        struct touch_point_t
        {
            u8  m_id;
            u8  m_size;
            u16 m_x;
            u16 m_y;
        };

        inline void touch_point_init(touch_point_t& tp, u8 _id, u16 _x, u16 _y, u16 _size)
        {
            tp.m_id   = _id;
            tp.m_x    = _x;
            tp.m_y    = _y;
            tp.m_size = _size;
        }

        void touch_point_transform(touch_point_t& tp, u16 width, u16 height, erotate_t rotation, emirror_t mirror);

        struct touch_t
        {
            u16   m_width;
            u16   m_height;
            u8    m_rotation;             // 0 = normal, 1 = rotate 90, 2 = rotate 180, 3 = rotate 270
            u8    m_mirror;               // 0 = normal, 1 = mirror X, 2 = mirror Y, 3 = mirror XY
            u16   m_polling_interval_ms;  // Polling interval in milliseconds
            u64   m_polling_timer_ms;     // Polling timer in milliseconds
            void* m_touch_panel;          // Pointer to the touch panel driver instance
            bool (*m_touch_panel_scan_fn)(touch_t& tp, touch_point_t* points, u8 max_touches, u8* num_touches);
        };

        // Initialize touchscreen
        // Note: don't call this directly, call the touch_init of a specific touch panel driver
        void touch_init(touch_t& tp, u16 width, u16 height, u16 polling_interval_ms, erotate_t rotation, emirror_t mirror);

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   false: no scan available
        //   true: scan available, touches updated
        bool touch_scan(touch_t& tp, u64 now_ms, touch_point_t* points, u8 max_touches, u8* num_touches);

    }  // namespace ntouch
}  // namespace ncore

#endif  // __RLCD_LIBTOUCH_TOUCH_H__
