#ifndef __RLCD_WCS_GT9XXX_H__
#define __RLCD_WCS_GT9XXX_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace ngt9xxx
    {
        struct device_t;

        device_t* create();
        u8        max_tps(device_t* dev);  // Get the maximum number of touch points supported by the device

        // GT9XXX device address
        enum gt9xxx_device_e
        {
            GT9XXX_DEV_ID = 0x14,  // GT9XXX device address
        };

        // GT9XXX I2C read/write commands
        enum gt9xxx_cmd_e
        {
            GT9XXX_CMD_WR = 0X28,  // I2C Write command
            GT9XXX_CMD_RD = 0X29,  // I2C Read command
        };

        // GT9XXX register definitions
        enum gt9xxx_reg_e
        {
            GT9XXX_CTRL_REG  = 0X8040,  // GT9XXX control register
            GT9XXX_CFGS_REG  = 0X8047,  // GT9XXX configuration start register
            GT9XXX_CHECK_REG = 0X80FF,  // GT9XXX checksum register
            GT9XXX_PID_REG   = 0X8140,  // GT9XXX product ID register

            GT9XXX_GSTID_REG = 0X814E,  // Current touch status detected by GT9XXX
            GT9XXX_TP1_REG   = 0X8150,  // Touch point 1 data address
            GT9XXX_TP2_REG   = 0X8158,  // Touch point 2 data address
            GT9XXX_TP3_REG   = 0X8160,  // Touch point 3 data address
            GT9XXX_TP4_REG   = 0X8168,  // Touch point 4 data address
            GT9XXX_TP5_REG   = 0X8170,  // Touch point 5 data address
            GT9XXX_TP6_REG   = 0X8178,  // Touch point 6 data address
            GT9XXX_TP7_REG   = 0X8180,  // Touch point 7 data address
            GT9XXX_TP8_REG   = 0X8188,  // Touch point 8 data address
            GT9XXX_TP9_REG   = 0X8190,  // Touch point 9 data address
            GT9XXX_TP10_REG  = 0X8198,  // Touch point 10 data address
        };

        bool write_reg(device_t* dev, u16 reg, u8* buf, u8 len);
        bool read_reg(device_t* dev, u16 reg, u8* buf, u8 len);
        bool read_tpx_reg(device_t* dev, u16 i, u8* buf, u8 len);

    }  // namespace ngt9xxx
}  // namespace ncore

#endif  // __RLCD_WCS_GT9XXX_H__
