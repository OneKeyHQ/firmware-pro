#include "status_bar.h"
#include "stdio.h"
#include "lvgl.h"
#include "ui_msg.h"
#include "user_assert.h"
#include "user_utils.h"
#include "cmsis_os2.h"

static lv_obj_t *g_statusBar;

void CreateStatusBar(void)
{
    g_statusBar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_statusBar, MY_DISP_HOR_RES, STATUS_BAR_HEIGHT);
    lv_obj_set_style_bg_color(g_statusBar, lv_color_black(), 0);
    lv_obj_set_style_border_width(g_statusBar, 0, 0);
    lv_obj_set_style_radius(g_statusBar, 0, 0);
    lv_obj_set_style_pad_all(g_statusBar, 0, 0);
    lv_obj_set_style_opa(g_statusBar, LV_OPA_COVER, 0);
}

void HandleStatusBarMsg(uint32_t code, void *data, uint32_t dataLen)
{
    UNUSED(data);
    UNUSED(dataLen);
    switch (code) {
    default:
        break;
    }
}

