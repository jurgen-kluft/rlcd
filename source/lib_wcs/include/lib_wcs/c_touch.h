#ifndef __RLCD_WCS_TOUCH_H__
#define __RLCD_WCS_TOUCH_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "lib_wcs/c_gt9xxx.h"
#include "lib_touch/c_touch.h"

namespace ncore
{
    namespace ntouch
    {
        struct touch_panel_t
        {
            u16 m_width;   // Screen width
            u16 m_height;  // Screen height

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

        static inline bool          tp_is_portrait(const touch_panel_t& tp) { return (tp.m_touchtype & 0x01) == 0; }
        static inline bool          tp_is_landscape(const touch_panel_t& tp) { return (tp.m_touchtype & 0x01) != 0; }
        static inline bool          tp_is_capacitive(const touch_panel_t& tp) { return (tp.m_touchtype & 0x80) != 0; }

        bool tp_init(touch_panel_t& tp, u16 width, u16 height, bool portrait = true);

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   0: no touch
        //   1: touch detected
        bool tp_scan(touch_panel_t& tp, u64 now_ms);

    }  // namespace ntouch
}  // namespace ncore

#endif  // __RLCD_WCS_TOUCH_H__
