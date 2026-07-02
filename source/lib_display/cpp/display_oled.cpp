#include "lib_display/display_databus.h"
#include "lib_display/display_gfx.h"
#include "lib_display/display_tft.h"
#include "lib_display/display_oled.h"

Arduino_OLED::Arduino_OLED(Arduino_DataBus *bus, int8_t rst, uint8_t r, int16_t w, int16_t h, uint8_t col_offset1, uint8_t row_offset1, uint8_t col_offset2, uint8_t row_offset2)
    : Arduino_TFT(bus, rst, r, false /* ips */, w, h, col_offset1, row_offset1, col_offset2, row_offset2)
{
}
