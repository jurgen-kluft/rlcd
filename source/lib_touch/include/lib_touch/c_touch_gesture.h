#ifndef __RLCD_TOUCH_GESTURE_H__
#define __RLCD_TOUCH_GESTURE_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

#include "lib_touch/c_touch.h"

namespace ncore
{
    namespace ntouch
    {
        // Supported gesture types (don't reorder or change values!)
        enum egesture_type_t
        {
            GT_NONE                   = 0,
            GT_TAP                    = 1,
            GT_DOUBLE_TAP             = 2,
            GT_DIR_LEFT               = 1 << 2,
            GT_DIR_RIGHT              = 2 << 2,
            GT_DIR_UP                 = 1 << 4,
            GT_DIR_DOWN               = 2 << 4,
            GT_SWIPE_ONE_FINGER       = 1 << 6,
            GT_SWIPE_TWO_FINGER       = 2 << 6,
            GT_ONE_FINGER_SWIPE_LEFT  = GT_DIR_LEFT | GT_SWIPE_ONE_FINGER,
            GT_ONE_FINGER_SWIPE_RIGHT = GT_DIR_RIGHT | GT_SWIPE_ONE_FINGER,
            GT_ONE_FINGER_SWIPE_UP    = GT_DIR_UP | GT_SWIPE_ONE_FINGER,
            GT_ONE_FINGER_SWIPE_DOWN  = GT_DIR_DOWN | GT_SWIPE_ONE_FINGER,
            GT_TWO_FINGER_SWIPE_LEFT  = GT_DIR_LEFT | GT_SWIPE_TWO_FINGER,
            GT_TWO_FINGER_SWIPE_RIGHT = GT_DIR_RIGHT | GT_SWIPE_TWO_FINGER,
            GT_TWO_FINGER_SWIPE_UP    = GT_DIR_UP | GT_SWIPE_TWO_FINGER,
            GT_TWO_FINGER_SWIPE_DOWN  = GT_DIR_DOWN | GT_SWIPE_TWO_FINGER,
        };
        const char* to_string(egesture_type_t gesture);  // Utility to get a text string of the gesture name

        static inline bool is_tap(egesture_type_t gt) { return gt == GT_TAP || gt == GT_DOUBLE_TAP; }
        static inline bool is_double_tap(egesture_type_t gt) { return gt == GT_DOUBLE_TAP; }
        static inline bool is_swipe(egesture_type_t gt) { return (gt & (GT_SWIPE_ONE_FINGER | GT_SWIPE_TWO_FINGER)) != 0; }
        static inline bool is_two_finger(egesture_type_t gt) { return (gt & GT_SWIPE_TWO_FINGER) != 0; }
        static inline bool is_swipe_up(egesture_type_t gt) { return (gt & GT_DIR_UP) != 0 && is_swipe(gt); }
        static inline bool is_swipe_down(egesture_type_t gt) { return (gt & GT_DIR_DOWN) != 0 && is_swipe(gt); }
        static inline bool is_swipe_left(egesture_type_t gt) { return (gt & GT_DIR_LEFT) != 0 && is_swipe(gt); }
        static inline bool is_swipe_right(egesture_type_t gt) { return (gt & GT_DIR_RIGHT) != 0 && is_swipe(gt); }

        // Configuration settings for tuning gestures
        struct gesture_config_t
        {
            i32 m_swipeThresholdPixels = 50;
            i32 m_tapMaxMovementPixels = 15;
            u64 m_tapTimeoutMs         = 250;
            u64 m_doubleTapGapMs       = 300;
        };

        struct touch_gesture_t
        {
            gesture_config_t m_cfg;
            u64              m_touchStartTime;
            u64              m_lastTapTime;
            touch_point_t    m_startP1;
            touch_point_t    m_startP2;
            touch_point_t    m_lastP1;
            touch_point_t    m_lastP2;
            bool             m_isTracking;
            i32              m_maxFingersDetected;
        };

        void            init_touch_gesture(touch_gesture_t& tg, gesture_config_t const& config);
        egesture_type_t update_touch_gesture(touch_gesture_t& tg, u64 now_ms, bool p1_present, touch_point_t const& p1, bool p2_present, touch_point_t const& p2);
        touch_point_t   get_single_tap_location(touch_gesture_t& tg);
        touch_point_t   get_double_tap_location(touch_gesture_t& tg);
        void            get_swipe_vector(touch_gesture_t& tg, touch_point_t& from, touch_point_t& to);
        i32             get_touch_point_distance(touch_point_t const& p1, touch_point_t const& p2);

    }  // namespace ntouch
}  // namespace ncore

#endif  // __RLCD_TOUCH_GESTURE_H__