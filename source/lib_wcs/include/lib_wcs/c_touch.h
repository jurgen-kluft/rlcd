#ifndef __RLCD_WCS_TOUCH_H__
#define __RLCD_WCS_TOUCH_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "lib_wcs/c_gt9xxx.h"

namespace ncore
{
    namespace ntouch
    {
        // Number of points supported by capacitive touch, fixed at 5 points
#define TP_CT_MAX_TOUCH 10

        struct touch_point_t
        {
            u16 m_x;  // X coordinate
            u16 m_y;  // Y coordinate
        };

        struct touch_panel_t
        {
            touch_point_t m_points[TP_CT_MAX_TOUCH];  // Capacitive touch supports up to 10 coordinate groups

            u16 m_width;   // Screen width
            u16 m_height;  // Screen height

            // Touch status
            // b15: 1 pressed / 0 released;
            // b14: 0 no touch point active; 1 touch point active.
            // b13~b10: reserved
            // b9~b0: number of pressed points on capacitive touchscreen
            //        (0 means not pressed, 1 means pressed)
            u16 m_touchstatus;

            // Additional parameter, used when left-right and up-down touch directions are fully inverted.
            // b0: 0, portrait mode (suitable when left-right is X and up-down is Y)
            //     1, landscape mode (suitable when left-right is Y and up-down is X)
            // b1~6: reserved.
            // b7: 0, resistive touch
            //     1, capacitive touch
            //
            u8 m_touchtype;

            u8 m_polling_interval;  // Control polling interval to reduce CPU usage
        };

        static inline bool          tp_is_pressed(const touch_panel_t& tp) { return (tp.m_touchstatus & 0x8000) != 0; }
        static inline bool          tp_is_portrait(const touch_panel_t& tp) { return (tp.m_touchtype & 0x01) == 0; }
        static inline bool          tp_is_landscape(const touch_panel_t& tp) { return (tp.m_touchtype & 0x01) != 0; }
        static inline bool          tp_is_point_active(const touch_panel_t& tp) { return (tp.m_touchstatus & 0x4000) != 0; }
        static inline u8            tp_get_touch_point_num(const touch_panel_t& tp) { return tp.m_touchstatus & 0x3FF; }
        static inline bool          tp_is_capacitive(const touch_panel_t& tp) { return (tp.m_touchtype & 0x80) != 0; }
        static inline bool          tp_is_valid_touch_point(const touch_panel_t& tp, u8 index) { return (index < TP_CT_MAX_TOUCH) && (tp.m_touchstatus & (1 << index)) != 0; }
        static inline touch_point_t tp_get_touch_point(const touch_panel_t& tp, u8 index) { return tp_is_valid_touch_point(tp, index) ? tp.m_points[index] : touch_point_t{0xFFFF, 0xFFFF}; }

        bool tp_init(touch_panel_t& tp, u16 width, u16 height, bool portrait = true);

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   0: no touch
        //   1: touch detected
        bool tp_scan(touch_panel_t& tp, u8 mode = 0);

    }  // namespace ntouch
}  // namespace ncore

#endif  // __RLCD_WCS_TOUCH_H__
