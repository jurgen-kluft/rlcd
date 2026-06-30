#include "Arduino.h"
#include <Wire.h>

#include "ccore/c_memory.h"
#include "lib_guition/c_touch.h"

namespace ncore
{
    namespace ntouch
    {

#define GT911_ADDR1 (u8)0x5D
#define GT911_ADDR2 (u8)0x14

// Real-time command (Write only)
#define GT911_COMMAND       (u16)0x8040
#define GT911_ESD_CHECK     (u16)0x8041
#define GT911_COMMAND_CHECK (u16)0x8046

#define GT911_STRETCH_R0              (u16)0x805E
#define GT911_STRETCH_R1              (u16)0x805F
#define GT911_STRETCH_R2              (u16)0x8060
#define GT911_STRETCH_RM              (u16)0x8061
#define GT911_DRV_GROUPA_NUM          (u16)0x8062
#define GT911_CONFIG_START            (u16)0x8047
#define GT911_CONFIG_VERSION          (u16)0x8047
#define GT911_X_OUTPUT_MAX_LOW        (u16)0x8048
#define GT911_X_OUTPUT_MAX_HIGH       (u16)0x8049
#define GT911_Y_OUTPUT_MAX_LOW        (u16)0x804A
#define GT911_Y_OUTPUT_MAX_HIGH       (u16)0x804B
#define GT911_TOUCH_NUMBER            (u16)0x804C
#define GT911_MODULE_SWITCH_1         (u16)0x804D
#define GT911_MODULE_SWITCH_2         (u16)0x804E
#define GT911_SHAKE_COUNT             (u16)0x804F
#define GT911_FILTER                  (u16)0x8050
#define GT911_LARGE_TOUCH             (u16)0x8051
#define GT911_NOISE_REDUCTION         (u16)0x8052
#define GT911_SCREEN_TOUCH_LEVEL      (u16)0x8053
#define GT911_SCREEN_RELEASE_LEVEL    (u16)0x8054
#define GT911_LOW_POWER_CONTROL       (u16)0x8055
#define GT911_REFRESH_RATE            (u16)0x8056
#define GT911_X_THRESHOLD             (u16)0x8057
#define GT911_Y_THRESHOLD             (u16)0x8058
#define GT911_SPACE_TOP_BOTTOM        (u16)0x805B
#define GT911_PANEL_TX_GAIN           (u16)0x806B
#define GT911_PANEL_RX_GAIN           (u16)0x806C
#define GT911_PANEL_DUMP_SHIFT        (u16)0x806D
#define GT911_DRV_FRAME_CONTROL       (u16)0x806E
#define GT911_CHARGING_LEVEL_UP       (u16)0x806F
#define GT911_MODULE_SWITCH3          (u16)0x8070
#define GT911_GESTURE_DIS             (u16)0X8071
#define GT911_GESTURE_LONG_PRESS_TIME (u16)0x8072
#define GT911_X_Y_SLOPE_ADJUST        (u16)0X8073
#define GT911_GESTURE_CONTROL         (u16)0X8074
#define GT911_GESTURE_SWITCH1         (u16)0X8075
#define GT911_GESTURE_SWITCH2         (u16)0X8076
#define GT911_GESTURE_REFRESH_RATE    (u16)0x8077
#define GT911_GESTURE_TOUCH_LEVEL     (u16)0x8078
#define GT911_NEWGREENWAKEUPLEVEL     (u16)0x8079
#define GT911_FREQ_HOPPING_START      (u16)0x807A
#define GT911_CONFIG_CHKSUM           (u16)0X80FF
#define GT911_CONFIG_FRESH            (u16)0X8100
#define GT911_CONFIG_SIZE             (u16)0xFF - 0x46

// Coordinate information
#define GT911_PRODUCT_ID       (u16)0X8140
#define GT911_FIRMWARE_VERSION (u16)0X8140
#define GT911_RESOLUTION       (u16)0X8140
#define GT911_VENDOR_ID        (u16)0X8140
#define GT911_IMFORMATION      (u16)0X8140
#define GT911_POINT_INFO       (u16)0X814E
#define GT911_POINT_1          (u16)0X814F
#define GT911_POINT_2          (u16)0X8157
#define GT911_POINT_3          (u16)0X815F
#define GT911_POINT_4          (u16)0X8167
#define GT911_POINT_5          (u16)0X816F
#define GT911_POINTS_REG       {GT911_POINT_1, GT911_POINT_2, GT911_POINT_3, GT911_POINT_4, GT911_POINT_5}

#define GUITION_TOUCH_I2C_ADDR     GT911_ADDR1
#define GUITION_TOUCH_I2C_SDA_GPIO 19
#define GUITION_TOUCH_I2C_SCL_GPIO 45
#define GUITION_TOUCH_RST_GPIO     GPIO_NUM_NC

#define GT911_I2C_ADDR GUITION_TOUCH_I2C_ADDR
#define GT911_I2C_SDA  GUITION_TOUCH_I2C_SDA_GPIO
#define GT911_I2C_SCL  GUITION_TOUCH_I2C_SCL_GPIO
#define GT911_I2C_RST  GUITION_TOUCH_RST_GPIO

        static void s_touch_setRotation(touch_panel_t& tp, u8 rot);
        static void s_touch_setResolution(touch_panel_t& tp, u16 _width, u16 _height);

        static void s_touch_init(touch_panel_t& tp)
        {
            tp.m_isLargeDetect = false;
            tp.m_touches       = 0;
            g_memset(tp.m_points, 0, sizeof(tp.m_points));
            tp.m_rotation = ROTATION_NORMAL;
            tp.m_width    = 480;
            tp.m_height   = 480;
            g_memset(tp.m_configBuf, 0, GT911_CONFIG_SIZE);

            s_touch_setRotation(tp, ROTATION_NORMAL);
            s_touch_setResolution(tp, tp.m_width, tp.m_height);

            Wire.begin(GUITION_TOUCH_I2C_SDA_GPIO, GUITION_TOUCH_I2C_SCL_GPIO);
        }

        static touch_point_t s_touchreadPoint(touch_panel_t& tp, u8* data);

        static void s_touch_writeByteData(u8 addr, u16 reg, u8 val);
        static u8   s_touch_readByteData(u8 addr, u16 reg);
        static void s_touch_writeBlockData(u8 addr, u16 reg, u8* val, u8 size);
        static void s_touch_readBlockData(u8 addr, u8* buf, u16 reg, u8 size);

        static void s_touch_calculateChecksum(touch_panel_t& tp)
        {
            u8 checksum;
            for (u8 i = 0; i < GT911_CONFIG_SIZE; i++)
            {
                checksum += tp.m_configBuf[i];
            }
            checksum                                                 = (~checksum) + 1;
            tp.m_configBuf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START] = checksum;
        }

        static void s_touch_reConfig(touch_panel_t& tp)
        {
            s_touch_calculateChecksum(tp);
            s_touch_writeByteData(GUITION_TOUCH_I2C_ADDR, GT911_CONFIG_CHKSUM, tp.m_configBuf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START]);
            s_touch_writeByteData(GUITION_TOUCH_I2C_ADDR, GT911_CONFIG_FRESH, 1);
        }

        static void s_touch_setRotation(touch_panel_t& tp, u8 rot) { tp.m_rotation = rot; }
        static void s_touch_setResolution(touch_panel_t& tp, u16 _width, u16 _height)
        {
            tp.m_configBuf[GT911_X_OUTPUT_MAX_LOW - GT911_CONFIG_START]  = lowByte(_width);
            tp.m_configBuf[GT911_X_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = highByte(_width);
            tp.m_configBuf[GT911_Y_OUTPUT_MAX_LOW - GT911_CONFIG_START]  = lowByte(_height);
            tp.m_configBuf[GT911_Y_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = highByte(_height);
            s_touch_reConfig(tp);
        }

        static void s_touch_read(touch_panel_t& tp)
        {
            u8  data[7];
            u8  id;
            u16 x, y, size;

            u8 pointInfo       = s_touch_readByteData(GUITION_TOUCH_I2C_ADDR, GT911_POINT_INFO);
            u8 bufferStatus    = pointInfo >> 7 & 1;
            u8 proximityValid  = pointInfo >> 5 & 1;
            u8 haveKey         = pointInfo >> 4 & 1;
            tp.m_isLargeDetect = pointInfo >> 6 & 1;
            tp.m_touches       = pointInfo & 0xF;
            if (bufferStatus == 1)
            {
                for (u8 i = 0; i < tp.m_touches; i++)
                {
                    s_touch_readBlockData(GUITION_TOUCH_I2C_ADDR, data, GT911_POINT_1 + i * 8, 7);
                    tp.m_points[i] = s_touchreadPoint(tp, data);
                }
            }
            s_touch_writeByteData(GUITION_TOUCH_I2C_ADDR, GT911_POINT_INFO, 0);
        }

        static touch_point_t s_touchreadPoint(touch_panel_t& tp, u8* data)
        {
            u16 temp;
            u8  id   = data[0];
            u16 x    = data[1] + (data[2] << 8);
            u16 y    = data[3] + (data[4] << 8);
            u16 size = data[5] + (data[6] << 8);
            switch (tp.m_rotation)
            {
                case ROTATION_NORMAL:
                    x = tp.m_width - x;
                    y = tp.m_height - y;
                    break;
                case ROTATION_LEFT:
                    temp = x;
                    x    = tp.m_width - y;
                    y    = temp;
                    break;
                case ROTATION_INVERTED:
                    x = x;
                    y = y;
                    break;
                case ROTATION_RIGHT:
                    temp = x;
                    x    = y;
                    y    = tp.m_height - temp;
                    break;
                default: break;
            }

            touch_point_t point;
            point.m_id   = id;
            point.m_size = size;
            point.m_x    = x;
            point.m_y    = y;
            return point;
        }

        static void s_touch_writeByteData(u8 addr, u16 reg, u8 val)
        {
            Wire.beginTransmission(addr);
            Wire.write(highByte(reg));
            Wire.write(lowByte(reg));
            Wire.write(val);
            Wire.endTransmission();
        }

        static u8 s_touch_readByteData(u8 addr, u16 reg)
        {
            u8 x;
            Wire.beginTransmission(addr);
            Wire.write(highByte(reg));
            Wire.write(lowByte(reg));
            Wire.endTransmission();
            Wire.requestFrom(addr, (u8)1);
            x = Wire.read();
            return x;
        }

        static void s_touch_writeBlockData(u8 addr, u16 reg, u8* val, u8 size)
        {
            Wire.beginTransmission(addr);
            Wire.write(highByte(reg));
            Wire.write(lowByte(reg));
            // Wire.write(val, size);
            for (u8 i = 0; i < size; i++)
            {
                Wire.write(val[i]);
            }
            Wire.endTransmission();
        }

        static void s_touch_readBlockData(u8 addr, u8* buf, u16 reg, u8 size)
        {
            Wire.beginTransmission(addr);
            Wire.write(highByte(reg));
            Wire.write(lowByte(reg));
            Wire.endTransmission();
            Wire.requestFrom(addr, size);
            for (u8 i = 0; i < size; i++)
            {
                buf[i] = Wire.read();
            }
        }

        bool tp_init(touch_panel_t& tp, u16 width, u16 height, bool portrait)
        {
            s_touch_init(tp);
            s_touch_setResolution(tp, width, height);
            if (portrait)
            {
                s_touch_setRotation(tp, ROTATION_NORMAL);
            }
            else
            {
                s_touch_setRotation(tp, ROTATION_LEFT);
            }
            return true;
        }

        // Scan touchscreen (polling mode)
        // return: current touch status
        //   0: no touch
        //   1: touch detected
        bool tp_scan(touch_panel_t& tp, u8 mode)
        {
            s_touch_read(tp);
            return tp_is_pressed(tp);
        }

    }  // namespace ntouch
}  // namespace ncore