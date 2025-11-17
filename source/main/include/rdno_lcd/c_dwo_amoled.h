#ifndef __RDNO_LCD_DWO_AMOLED_H__
#define __RDNO_LCD_DWO_AMOLED_H__

namespace ndwo
{
    typedef void (*notify_flush_ready_cb)(void* user_ctx);
    struct user_notify_flush_ready_t
    {
        notify_flush_ready_cb m_cb;
        void*                 m_user_ctx;
    };

    struct panel_handles_t
    {
        void* panel_handle;
        void* touch_handle;
    };

    bool init_dwo_display(int32_t h_res, int32_t v_res, int8_t bpp, user_notify_flush_ready_t* user_notify_flush_ready, panel_handles_t* out_handles);
}  // namespace ndwo

#endif  // __RDNO_LCD_DWO_AMOLED_H__
