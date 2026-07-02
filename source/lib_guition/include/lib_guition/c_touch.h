#ifndef __RLCD_GUITION_TOUCH_H__
#define __RLCD_GUITION_TOUCH_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace ntouch
    {
#define TP_CT_MAX_TOUCH 5  // Number of points supported by capacitive touch, fixed at 5 points

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

        struct touch_panel_t
        {
            u16           m_width;
            u16           m_height;
            u16           m_touches;
            u8            m_rotation;  // 0 = normal, 1 = rotate 90, 2 = rotate 180, 3 = rotate 270
            u8            m_mirror;    // 0 = normal, 1 = mirror X, 2 = mirror Y, 3 = mirror XY
            u16           m_touch_status;
            touch_point_t m_points[TP_CT_MAX_TOUCH];
            u64           m_polling_interval;
        };

        static inline bool                 tp_is_pressed(const touch_panel_t& tp) { return (tp.m_touches > 0); }
        static inline bool                 tp_is_point_active(const touch_panel_t& tp) { return (tp_is_pressed(tp) && tp.m_touches > 0); }
        static inline u8                   tp_get_touch_point_num(const touch_panel_t& tp) { return tp.m_touches; }
        static inline bool                 tp_is_valid_touch_point(const touch_panel_t& tp, u8 index) { return (index < TP_CT_MAX_TOUCH) && (tp.m_touches & (1 << index)) != 0; }
        static inline const touch_point_t* tp_get_touch_point(const touch_panel_t& tp, u8 index) { return tp_is_valid_touch_point(tp, index) ? &tp.m_points[index] : nullptr; }

        bool tp_init(touch_panel_t& tp, u16 width, u16 height, erotate_t rotation = cTP_ROTATE_0, emirror_t mirror = cTP_MIRROR_NONE);

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   0: no touch
        //   1: touch detected
        bool tp_scan(touch_panel_t& tp, u64 now);

    }  // namespace ntouch
}  // namespace ncore

#endif  // __RLCD_GUITION_TOUCH_H__
