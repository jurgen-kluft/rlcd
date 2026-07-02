#include "Arduino.h"
#include "Wire.h"

#define GT911_ADDR1 (uint8_t)0x5D
#define GT911_ADDR2 (uint8_t)0x14

// #define ROTATION_LEFT     (uint8_t)0
// #define ROTATION_INVERTED (uint8_t)1
// #define ROTATION_RIGHT    (uint8_t)2
// #define ROTATION_NORMAL   (uint8_t)3

// Real-time command (Write only)
#define GT911_COMMAND       (uint16_t)0x8040
#define GT911_ESD_CHECK     (uint16_t)0x8041
#define GT911_COMMAND_CHECK (uint16_t)0x8046

#define GT911_STRETCH_R0              (uint16_t)0x805E
#define GT911_STRETCH_R1              (uint16_t)0x805F
#define GT911_STRETCH_R2              (uint16_t)0x8060
#define GT911_STRETCH_RM              (uint16_t)0x8061
#define GT911_DRV_GROUPA_NUM          (uint16_t)0x8062
#define GT911_CONFIG_START            (uint16_t)0x8047
#define GT911_CONFIG_VERSION          (uint16_t)0x8047
#define GT911_X_OUTPUT_MAX_LOW        (uint16_t)0x8048
#define GT911_X_OUTPUT_MAX_HIGH       (uint16_t)0x8049
#define GT911_Y_OUTPUT_MAX_LOW        (uint16_t)0x804A
#define GT911_Y_OUTPUT_MAX_HIGH       (uint16_t)0x804B
#define GT911_TOUCH_NUMBER            (uint16_t)0x804C
#define GT911_MODULE_SWITCH_1         (uint16_t)0x804D
#define GT911_MODULE_SWITCH_2         (uint16_t)0x804E
#define GT911_SHAKE_COUNT             (uint16_t)0x804F
#define GT911_FILTER                  (uint16_t)0x8050
#define GT911_LARGE_TOUCH             (uint16_t)0x8051
#define GT911_NOISE_REDUCTION         (uint16_t)0x8052
#define GT911_SCREEN_TOUCH_LEVEL      (uint16_t)0x8053
#define GT911_SCREEN_RELEASE_LEVEL    (uint16_t)0x8054
#define GT911_LOW_POWER_CONTROL       (uint16_t)0x8055
#define GT911_REFRESH_RATE            (uint16_t)0x8056
#define GT911_X_THRESHOLD             (uint16_t)0x8057
#define GT911_Y_THRESHOLD             (uint16_t)0x8058
#define GT911_SPACE_TOP_BOTTOM        (uint16_t)0x805B
#define GT911_PANEL_TX_GAIN           (uint16_t)0x806B
#define GT911_PANEL_RX_GAIN           (uint16_t)0x806C
#define GT911_PANEL_DUMP_SHIFT        (uint16_t)0x806D
#define GT911_DRV_FRAME_CONTROL       (uint16_t)0x806E
#define GT911_CHARGING_LEVEL_UP       (uint16_t)0x806F
#define GT911_MODULE_SWITCH3          (uint16_t)0x8070
#define GT911_GESTURE_DIS             (uint16_t)0X8071
#define GT911_GESTURE_LONG_PRESS_TIME (uint16_t)0x8072
#define GT911_X_Y_SLOPE_ADJUST        (uint16_t)0X8073
#define GT911_GESTURE_CONTROL         (uint16_t)0X8074
#define GT911_GESTURE_SWITCH1         (uint16_t)0X8075
#define GT911_GESTURE_SWITCH2         (uint16_t)0X8076
#define GT911_GESTURE_REFRESH_RATE    (uint16_t)0x8077
#define GT911_GESTURE_TOUCH_LEVEL     (uint16_t)0x8078
#define GT911_NEWGREENWAKEUPLEVEL     (uint16_t)0x8079
#define GT911_FREQ_HOPPING_START      (uint16_t)0x807A
#define GT911_CONFIG_CHKSUM           (uint16_t)0X80FF
#define GT911_CONFIG_FRESH            (uint16_t)0X8100
#define GT911_CONFIG_SIZE             (uint16_t)0xFF - 0x46
// Coordinate information
#define GT911_PRODUCT_ID       (uint16_t)0X8140
#define GT911_FIRMWARE_VERSION (uint16_t)0X8140
#define GT911_RESOLUTION       (uint16_t)0X8140
#define GT911_VENDOR_ID        (uint16_t)0X8140
#define GT911_IMFORMATION      (uint16_t)0X8140
#define GT911_POINT_INFO       (uint16_t)0X814E
#define GT911_POINT_1          (uint16_t)0X814F
#define GT911_POINT_2          (uint16_t)0X8157
#define GT911_POINT_3          (uint16_t)0X815F
#define GT911_POINT_4          (uint16_t)0X8167
#define GT911_POINT_5          (uint16_t)0X816F
#define GT911_POINTS_REG       {GT911_POINT_1, GT911_POINT_2, GT911_POINT_3, GT911_POINT_4, GT911_POINT_5}

struct touch_point_t
{
    uint8_t  id;
    uint16_t x;
    uint16_t y;
    uint8_t  size;
};

static void s_touch_point_init(touch_point_t* tp, uint8_t _id, uint16_t _x, uint16_t _y, uint16_t _size)
{
    tp->id   = _id;
    tp->x    = _x;
    tp->y    = _y;
    tp->size = _size;
}

struct touch_gt911_t
{
    uint8_t  rotation = ROTATION_NORMAL;
    uint8_t  addr;
    uint8_t  pinSda;
    uint8_t  pinScl;
    uint8_t  pinRst;
    uint16_t width;
    uint16_t height;
    uint8_t  configBuf[GT911_CONFIG_SIZE];

    uint8_t       isLargeDetect;
    uint8_t       touches   = 0;
    bool          isTouched = false;
    touch_point_t points[5];

    Wire* m_wire;
};

static void          s_tp_calculateChecksum(touch_gt911_t* tp);
static void          s_tp_reConfig(touch_gt911_t* tp);
static touch_point_t s_tp_readPoint(touch_gt911_t* tp, uint8_t* data);
static void          s_tp_writeByteData(touch_gt911_t* tp, uint16_t reg, uint8_t val);
static uint8_t       s_tp_readByteData(touch_gt911_t* tp, uint16_t reg);
static void          s_tp_writeBlockData(touch_gt911_t* tp, uint16_t reg, uint8_t* val, uint8_t size);
static void          s_tp_readBlockData(touch_gt911_t* tp, uint8_t* buf, uint16_t reg, uint8_t size);

static void s_tp_initialize(touch_gt911_t* tp, uint8_t _sda, uint8_t _scl, uint8_t _int, uint8_t _rst, uint16_t _width, uint16_t _height)
{
    tp->pinSda = _sda;
    tp->pinScl = _scl;
    tp->pinRst = _rst;
    tp->width  = _width;
    tp->height = _height;

    tp->m_wire = new Wire();
    tp->m_wire->begin(_sda, _scl);
}

static void s_tp_begin(touch_gt911_t* tp, uint8_t _addr)
{
    tp->addr = _addr;
    tp->m_wire->begin(tp->pinSda, tp->pinScl);
}

static void s_tp_calculateChecksum(touch_gt911_t* tp)
{
    uint8_t checksum;
    for (uint8_t i = 0; i < GT911_CONFIG_SIZE; i++)
    {
        checksum += tp->configBuf[i];
    }
    checksum                                                = (~checksum) + 1;
    tp->configBuf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START] = checksum;
}

static void s_tp_reConfig(touch_gt911_t* tp)
{
    s_tp_calculateChecksum(tp);
    s_tp_writeByteData(tp, GT911_CONFIG_CHKSUM, tp->configBuf[GT911_CONFIG_CHKSUM - GT911_CONFIG_START]);
    s_tp_writeByteData(tp, GT911_CONFIG_FRESH, 1);
}

static void s_tp_setRotation(touch_gt911_t* tp, uint8_t rot) { tp->rotation = rot; }

static void s_tp_setResolution(touch_gt911_t* tp, uint16_t _width, uint16_t _height)
{
    tp->configBuf[GT911_X_OUTPUT_MAX_LOW - GT911_CONFIG_START]  = lowByte(_width);
    tp->configBuf[GT911_X_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = highByte(_width);
    tp->configBuf[GT911_Y_OUTPUT_MAX_LOW - GT911_CONFIG_START]  = lowByte(_height);
    tp->configBuf[GT911_Y_OUTPUT_MAX_HIGH - GT911_CONFIG_START] = highByte(_height);
    s_tp_reConfig(tp);
}

static bool s_tp_read(touch_gt911_t* tp)
{
    uint8_t pointInfo      = s_tp_readByteData(tp, GT911_POINT_INFO);
    uint8_t bufferStatus   = pointInfo >> 7 & 1;
    uint8_t proximityValid = pointInfo >> 5 & 1;
    uint8_t haveKey        = pointInfo >> 4 & 1;
    tp->isLargeDetect      = pointInfo >> 6 & 1;
    tp->touches            = pointInfo & 0xF;
    tp->isTouched          = tp->touches > 0;
    if (bufferStatus == 1 && tp->isTouched)
    {
        uint8_t data[7];
        for (uint8_t i = 0; i < tp->touches; i++)
        {
            s_tp_readBlockData(tp, data, GT911_POINT_1 + i * 8, 7);
            tp->points[i] = s_tp_readPoint(tp, data);
        }
        return true;
    }
    s_tp_writeByteData(tp, GT911_POINT_INFO, 0);
    return false;
}

touch_point_t s_tp_readPoint(touch_gt911_t* tp, uint8_t* data)
{
    uint16_t temp;
    uint8_t  id   = data[0];
    uint16_t x    = data[1] + (data[2] << 8);
    uint16_t y    = data[3] + (data[4] << 8);
    uint16_t size = data[5] + (data[6] << 8);
    switch (tp->rotation)
    {
        case ROTATION_NORMAL:
            x = tp->width - x;
            y = tp->height - y;
            break;
        case ROTATION_LEFT:
            temp = x;
            x    = tp->width - y;
            y    = temp;
            break;
        case ROTATION_INVERTED:
            x = x;
            y = y;
            break;
        case ROTATION_RIGHT:
            temp = x;
            x    = y;
            y    = tp->height - temp;
            break;
        default: break;
    }

    touch_point_t point;
    s_touch_point_init(&point, id, x, y, size);
    return point;
}

static void s_tp_writeByteData(touch_gt911_t* tp, uint16_t reg, uint8_t val)
{
    tp->m_wire->beginTransmission(tp->addr);
    tp->m_wire->write(highByte(reg));
    tp->m_wire->write(lowByte(reg));
    tp->m_wire->write(val);
    tp->m_wire->endTransmission();
}

static uint8_t s_tp_readByteData(touch_gt911_t* tp, uint16_t reg)
{
    uint8_t x;
    tp->m_wire->beginTransmission(tp->addr);
    tp->m_wire->write(highByte(reg));
    tp->m_wire->write(lowByte(reg));
    tp->m_wire->endTransmission();
    tp->m_wire->requestFrom(tp->addr, (uint8_t)1);
    x = tp->m_wire->read();
    return x;
}

static void s_tp_writeBlockData(touch_gt911_t* tp, uint16_t reg, uint8_t* val, uint8_t size)
{
    tp->m_wire->beginTransmission(tp->addr);
    tp->m_wire->write(highByte(reg));
    tp->m_wire->write(lowByte(reg));
    // Wire.write(val, size);
    for (uint8_t i = 0; i < size; i++)
    {
        tp->m_wire->write(val[i]);
    }
    tp->m_wire->endTransmission();
}

static void s_tp_readBlockData(touch_gt911_t* tp, uint8_t* buf, uint16_t reg, uint8_t size)
{
    tp->m_wire->beginTransmission(tp->addr);
    tp->m_wire->write(highByte(reg));
    tp->m_wire->write(lowByte(reg));
    tp->m_wire->endTransmission();
    tp->m_wire->requestFrom(tp->addr, size);
    for (uint8_t i = 0; i < size; i++)
    {
        buf[i] = tp->m_wire->read();
    }
}

#define TOUCH_GT911
#define TOUCH_GT911_SCL      45
#define TOUCH_GT911_SDA      19
#define TOUCH_GT911_INT      -1
#define TOUCH_GT911_RST      -1
#define TOUCH_GT911_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1         480
#define TOUCH_MAP_X2         0
#define TOUCH_MAP_Y1         480
#define TOUCH_MAP_Y2         0

static int touch_last_x = 0, touch_last_y = 0;

static touch_gt911_t g_touch_panel = nullptr;

void touch_init()
{
    g_touch_panel = new touch_gt911_t();
    s_tp_initialize(g_touch_panel, TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));
    s_tp_begin(g_touch_panel);
    s_tp_setRotation(g_touch_panel, TOUCH_GT911_ROTATION);
}

bool touch_has_signal() { return true; }

static ncore::u64 interval = 0;
bool              touch_touched(ncore::u64 now_ms)
{
    if (now_ms > interval)  // Debounce interval
    {
        interval = now_ms + 50;  // Set next check time (50ms debounce)

        s_tp_read(g_touch_panel);
        if (g_touch_panel->isTouched)
        {
#if defined(TOUCH_SWAP_XY)
            touch_last_x = map(g_touch_panel->points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
            touch_last_y = map(g_touch_panel->points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
#else
            touch_last_x = map(g_touch_panel->points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, 480 - 1);
            touch_last_y = map(g_touch_panel->points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, 480 - 1);
#endif
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

bool touch_released() { return true; }

namespace ncore
{

}  // namespace ncore
