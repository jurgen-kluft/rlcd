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

#define ROTATION_LEFT     (u8)0
#define ROTATION_INVERTED (u8)1
#define ROTATION_RIGHT    (u8)2
#define ROTATION_NORMAL   (u8)3

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
            u8            m_isLargeDetect;
            u8            m_touches;
            u8            m_rotation;
            u8            m_reserved;
            touch_point_t m_points[TP_CT_MAX_TOUCH];
            u8            m_configBuf[256 - 38];
        };

        static inline bool          tp_is_pressed(const touch_panel_t& tp) { return (tp.m_touches > 0); }
        static inline bool          tp_is_portrait(const touch_panel_t& tp) { return (tp.m_rotation == ROTATION_NORMAL || tp.m_rotation == ROTATION_INVERTED); }
        static inline bool          tp_is_landscape(const touch_panel_t& tp) { return (tp.m_rotation == ROTATION_LEFT || tp.m_rotation == ROTATION_RIGHT); }
        static inline bool          tp_is_point_active(const touch_panel_t& tp) { return (tp_is_pressed(tp) && tp.m_touches > 0); }
        static inline u8            tp_get_touch_point_num(const touch_panel_t& tp) { return tp.m_touches; }
        static inline bool          tp_is_capacitive(const touch_panel_t& tp) { return true; }
        static inline bool          tp_is_valid_touch_point(const touch_panel_t& tp, u8 index) { return (index < TP_CT_MAX_TOUCH) && (tp.m_touches & (1 << index)) != 0; }
        static inline touch_point_t tp_get_touch_point(const touch_panel_t& tp, u8 index) { return tp_is_valid_touch_point(tp, index) ? tp.m_points[index] : touch_point_t{0, 0, 0xFFFF, 0xFFFF}; }

        bool tp_init(touch_panel_t& tp, u16 width, u16 height, bool portrait = true);

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   0: no touch
        //   1: touch detected
        bool tp_scan(touch_panel_t& tp, u8 mode = 0);

    }  // namespace ntouch
}  // namespace ncore

#endif  // __RLCD_GUITION_TOUCH_H__
