#ifndef __RLCD_GUITION_RELAY_H__
#define __RLCD_GUITION_RELAY_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nrelay
    {
        enum erelay_t
        {
            RELAY_1 = 0,
            RELAY_2 = 1,
            RELAY_3 = 2,
        };

        void relay_init();
        void relay_on(erelay_t relay_id);
        void relay_off(erelay_t relay_id);

    }  // namespace nrelay
}  // namespace ncore

#endif  // __RLCD_GUITION_RELAY_H__
