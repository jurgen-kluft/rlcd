#ifndef __RLCD_WCS_I2C_H__
#define __RLCD_WCS_I2C_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace ni2c
    {
        struct i2c_master_dev_t;

        i2c_master_dev_t* i2c_master_dev_create(i2c_master_bus_handle_t bus, u8 sda, u8 scl);

    }  // namespace ni2c
}  // namespace ncore

#endif  // __RLCD_WCS_I2C_H__
