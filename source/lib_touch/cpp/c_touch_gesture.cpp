#include "ccore/c_math.h"

#include "lib_touch/c_touch_gesture.h"

namespace ncore
{
    namespace ntouch
    {
        // Evaluate swipe direction based on the movement vector and number of fingers, returns GT_NONE if movement doesn't meet swipe thresholds
        static egesture_type_t s_evaluate_swipe(touch_gesture_t& tg, i32 dx, i32 dy, i32 fingers)
        {
            if (math::abs(dx) > math::abs(dy))
            {
                if (math::abs(dx) >= tg.m_cfg.m_swipeThresholdPixels)
                {
                    egesture_type_t gt = (fingers == 2) ? GT_TWO_FINGER_SWIPE_LEFT : GT_ONE_FINGER_SWIPE_LEFT;
                    return (dx > 0) ? (egesture_type_t)(gt + 1) : gt;
                }
            }
            else
            {
                if (math::abs(dy) >= tg.m_cfg.m_swipeThresholdPixels)
                {
                    egesture_type_t gt = (fingers == 2) ? GT_TWO_FINGER_SWIPE_UP : GT_ONE_FINGER_SWIPE_UP;
                    return (dy > 0) ? (egesture_type_t)(gt + 1) : gt;
                }
            }
            return GT_NONE;
        }

        // Constructor copies configuration settings
        void init_touch_gesture(touch_gesture_t& tg, gesture_config_t const& config)
        {
            tg.m_cfg                = (config);
            tg.m_touchStartTime     = (0);
            tg.m_lastTapTime        = (0);
            tg.m_isTracking         = (false);
            tg.m_maxFingersDetected = (0);
            tg.m_startP1            = {0, 0};
            tg.m_startP2            = {0, 0};
            tg.m_lastP1             = {0, 0};
            tg.m_lastP2             = {0, 0};
        }

        egesture_type_t update_touch_gesture(touch_gesture_t& tg, u64 now_ms, bool p1_present, touch_point_t const& p1, bool p2_present, touch_point_t const& p2)
        {
            egesture_type_t detectedGesture = GT_NONE;

            // 1. Capture Touch Initial State
            if ((p1_present || p2_present) && !tg.m_isTracking)
            {
                tg.m_touchStartTime     = now_ms;
                tg.m_startP1            = p1;
                tg.m_startP2            = p2;
                tg.m_isTracking         = true;
                tg.m_maxFingersDetected = (p1_present && p2_present) ? 2 : 1;

                tg.m_lastP1 = p1;
                tg.m_lastP2 = p2;
                return GT_NONE;
            }

            // Upgrade tracking to 2 fingers if second finger lands midway through
            if (tg.m_isTracking && p1_present && p2_present)
            {
                if (tg.m_maxFingersDetected < 2)
                {
                    tg.m_maxFingersDetected = 2;
                    tg.m_startP2            = p2;  // Set start position for finger 2 when it appears
                }
            }

            // Keep memory of the last active points before release
            if (p1_present)
                tg.m_lastP1 = p1;
            if (p2_present)
                tg.m_lastP2 = p2;

            // 2. Process Release Event
            if (!p1_present && !p2_present && tg.m_isTracking)
            {
                const u64 duration = now_ms - tg.m_touchStartTime;
                tg.m_isTracking    = false;

                // Calculate absolute movement distance for primary finger
                const i32 deltaX1 = tg.m_lastP1.m_x - tg.m_startP1.m_x;
                const i32 deltaY1 = tg.m_lastP1.m_y - tg.m_startP1.m_y;

                // Evaluate Tap or Double Tap
                if (duration < tg.m_cfg.m_tapTimeoutMs && math::abs(deltaX1) < tg.m_cfg.m_tapMaxMovementPixels && math::abs(deltaY1) < tg.m_cfg.m_tapMaxMovementPixels)
                {
                    if (now_ms - tg.m_lastTapTime < tg.m_cfg.m_doubleTapGapMs)
                    {
                        detectedGesture  = GT_DOUBLE_TAP;
                        tg.m_lastTapTime = 0;  // Prevent triple-tap cascade
                    }
                    else
                    {
                        detectedGesture  = GT_TAP;
                        tg.m_lastTapTime = now_ms;
                    }
                    return detectedGesture;
                }

                // Evaluate Swipes
                switch (tg.m_maxFingersDetected)
                {
                    case 1: detectedGesture = s_evaluate_swipe(tg, deltaX1, deltaY1, 1); break;
                    case 2: detectedGesture = s_evaluate_swipe(tg, deltaX1, deltaY1, 2); break;  // Evaluates based on the primary vector of the swipe gesture
                    default: detectedGesture = GT_NONE; break;
                }
            }

            return detectedGesture;
        }

        touch_point_t get_single_tap_location(touch_gesture_t& tg) { return tg.m_lastP1; }
        touch_point_t get_double_tap_location(touch_gesture_t& tg) { return tg.m_lastP1; }
        void          get_swipe_vector(touch_gesture_t& tg, touch_point_t& from, touch_point_t& to)
        {
            from = tg.m_startP1;
            to   = tg.m_lastP1;
        }
        i32 get_touch_point_distance(touch_point_t const& p1, touch_point_t const& p2)
        {
            i32 dx = p2.m_x - p1.m_x;
            i32 dy = p2.m_y - p1.m_y;
            return (i32)math::sqrtf(static_cast<f32>(dx * dx + dy * dy));
        }

        const char* to_string(egesture_type_t gesture)
        {
            switch (gesture)
            {
                case GT_NONE: return "None";
                case GT_TAP: return "Tap";
                case GT_DOUBLE_TAP: return "Double Tap";
                case GT_ONE_FINGER_SWIPE_LEFT: return "Swipe Left";
                case GT_ONE_FINGER_SWIPE_RIGHT: return "Swipe Right";
                case GT_ONE_FINGER_SWIPE_UP: return "Swipe Up";
                case GT_ONE_FINGER_SWIPE_DOWN: return "Swipe Down";
                case GT_TWO_FINGER_SWIPE_LEFT: return "2-Finger Swipe Left";
                case GT_TWO_FINGER_SWIPE_RIGHT: return "2-Finger Swipe Right";
                case GT_TWO_FINGER_SWIPE_UP: return "2-Finger Swipe Up";
                case GT_TWO_FINGER_SWIPE_DOWN: return "2-Finger Swipe Down";
                default: return "Unknown Gesture";
            }
        }

    }  // namespace ntouch
}  // namespace ncore
