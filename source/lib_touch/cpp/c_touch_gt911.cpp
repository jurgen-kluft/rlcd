#include "Arduino.h"
#include "Wire.h"

#include "lib_touch/c_touch.h"

namespace ncore
{
    namespace ntouch
    {
#define ROTATION_LEFT     (u8)0
#define ROTATION_INVERTED (u8)1
#define ROTATION_RIGHT    (u8)2
#define ROTATION_NORMAL   (u8)3

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

        namespace nwire
        {
            static void s_writeByteData(Wire* wire, u8 addr, u16 reg, u8 val)
            {
                wire->beginTransmission(addr);
                wire->write(highByte(reg));
                wire->write(lowByte(reg));
                wire->write(val);
                wire->endTransmission();
            }

            static u8 s_readByteData(Wire* wire, u8 addr, u16 reg)
            {
                u8 x;
                wire->beginTransmission(addr);
                wire->write(highByte(reg));
                wire->write(lowByte(reg));
                wire->endTransmission();
                wire->requestFrom(addr, (u8)1);
                x = wire->read();
                return x;
            }

            static void s_writeBlockData(Wire* wire, u8 addr, u16 reg, u8* val, u8 size)
            {
                wire->beginTransmission(addr);
                wire->write(highByte(reg));
                wire->write(lowByte(reg));
                // Wire.write(val, size);
                for (u8 i = 0; i < size; i++)
                {
                    wire->write(val[i]);
                }
                wire->endTransmission();
            }

            static void s_readBlockData(Wire* wire, u8 addr, u8* buf, u16 reg, u8 size)
            {
                wire->beginTransmission(addr);
                wire->write(highByte(reg));
                wire->write(lowByte(reg));
                wire->endTransmission();
                wire->requestFrom(addr, size);
                for (u8 i = 0; i < size; i++)
                {
                    buf[i] = wire->read();
                }
            }
        }  // namespace nwire

        struct touch_panel_gt911_t
        {
            u8   addr;
            u8   pinSda;
            u8   pinScl;
            u8   pinRst;
            u8   configBuf[GT911_CONFIG_SIZE];
            Wire m_wire;
        };

        static void s_tp_gt911_begin(touch_panel_gt911_t* tp, u8 _addr)
        {
            tp->addr = _addr;
            tp->m_wire.begin(tp->pinSda, tp->pinScl);
        }

        static void s_tp_gt911_calculateChecksum(touch_panel_gt911_t* tp)
        {
            u8 checksum;
            for (u8 i = 0; i < GT911_CONFIG_SIZE; i++)
            {
                checksum += tp->configBuf[i];
            }
            checksum                                                = (~checksum) + 1;
            tp->configBuf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START] = checksum;
        }

        static void s_tp_gt911_reconfig(touch_panel_gt911_t* tp)
        {
            s_tp_gt911_calculateChecksum(tp);
            nwire::s_writeByteData(&tp->m_wire, tp->addr, GT911_CONFIG_CHKSUM, tp->configBuf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START]);
            nwire::s_writeByteData(&tp->m_wire, tp->addr, GT911_CONFIG_FRESH, 1);
        }

        static void s_tp_gt911_setResolution(touch_panel_gt911_t* tp, u16 _width, u16 _height)
        {
            tp->configBuf[GT911_X_OUTPUT_MAX_LOW - GT911_CONFIG_START]  = lowByte(_width);
            tp->configBuf[GT911_X_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = highByte(_width);
            tp->configBuf[GT911_Y_OUTPUT_MAX_LOW - GT911_CONFIG_START]  = lowByte(_height);
            tp->configBuf[GT911_Y_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = highByte(_height);
            s_tp_gt911_reconfig(tp);
        }

        static inline u8  s_tp_gt911_read_id(u8* data) { return data[0]; }
        static inline u16 s_tp_gt911_read_x(u8* data) { return (u16)(data[1]) + (u16)(data[2] << 8); }
        static inline u16 s_tp_gt911_read_y(u8* data) { return (u16)(data[3]) + (u16)(data[4] << 8); }
        static inline u16 s_tp_gt911_read_size(u8* data) { return (u16)(data[5]) + (u16)(data[6] << 8); }

        static bool s_tp_gt911_read(touch_t& t, touch_point_t* points, u8* touches)
        {
            touch_panel_gt911_t* tp = (touch_panel_gt911_t*)t.m_touch_panel;

            u8 pointInfo      = nwire::s_readByteData(&tp->m_wire, tp->addr, GT911_POINT_INFO);
            u8 bufferStatus   = pointInfo >> 7 & 1;
            u8 proximityValid = pointInfo >> 5 & 1;
            u8 haveKey        = pointInfo >> 4 & 1;
            u8 isLargeDetect  = pointInfo >> 6 & 1;
            *touches          = pointInfo & 0xF;
            if (bufferStatus == 1 && (*touches > 0))
            {
                u8 data[7];
                for (u8 i = 0; i < *touches; i++)
                {
                    nwire::s_readBlockData(&tp->m_wire, tp->addr, data, GT911_POINT_1 + i * 8, 7);
                    const u16 x    = s_tp_gt911_read_x(data);
                    const u16 y    = s_tp_gt911_read_y(data);
                    const u8  id   = s_tp_gt911_read_id(data);
                    const u16 size = s_tp_gt911_read_size(data);
                    touch_point_init(points[i], id, x, y, size);
                    touch_point_transform(points[i], t.m_width, t.m_height, (erotate_t)t.m_rotation, (emirror_t)t.m_mirror);
                }
                return true;
            }
            nwire::s_writeByteData(&tp->m_wire, tp->addr, GT911_POINT_INFO, 0);
            return false;
        }

        static touch_panel_gt911_t* s_tp_gt911_create(u8 _addr, u8 _sda, u8 _scl, u8 _int, u8 _rst, u16 _width, u16 _height)
        {
            touch_panel_gt911_t* tp = new touch_panel_gt911_t();

            tp->addr   = _addr;
            tp->pinSda = _sda;
            tp->pinScl = _scl;
            tp->pinRst = _rst;

            tp->m_wire = Wire();
            tp->m_wire.begin(_sda, _scl);

            return tp;
        }

// GT911 touch panel configuration for 4'' inch 480x480 panel from Guition
#define TOUCH_GT911_ADDR1    0x5D
#define TOUCH_GT911_ADDR2    0x14
#define TOUCH_GT911_SCL      45
#define TOUCH_GT911_SDA      19
#define TOUCH_GT911_INT      -1
#define TOUCH_GT911_RST      -1
#define TOUCH_GT911_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_WIDTH      480
#define TOUCH_MAP_HEIGHT     480

        void touch_gt911_init(touch_t& t, u16 width, u16 height, u64 polling_interval_ms, u8 i2c_addr, i8 sda_pin, i8 scl_pin, i8 int_pin, i8 rst_pin, erotate_t rotation, emirror_t mirror)
        {
            touch_init(t, width, height, polling_interval_ms, rotation, mirror);

            touch_panel_gt911_t* touch_panel_gt911 = s_tp_gt911_create(i2c_addr, sda_pin, scl_pin, int_pin, rst_pin, width, height);
            s_tp_gt911_begin(touch_panel_gt911);

            t.m_touch_panel         = touch_panel_gt911;
            t.m_touch_panel_scan_fn = s_tp_gt911_read;
        }

    }  // namespace ntouch
}  // namespace ncore
