#ifndef __RDNO_LCD_DWO_AMOLED_H__
#define __RDNO_LCD_DWO_AMOLED_H__
#include "rdno_core/c_target.h"
#ifdef USE_PRAGMA_ONCE
#    pragma once
#endif

void init_dwo_display(int32_t h_res, int32_t v_res, int32_t bpp, esp_lcd_panel_io_color_trans_done_cb_t notify_flush_ready, void* notify_flush_ready_user_ctx);

#endif  // __RDNO_LCD_DWO_AMOLED_H__
