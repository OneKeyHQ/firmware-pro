#include "user_menu.h"
#include "lvgl.h"
#include "status_bar.h"
#include "images_declare.h"
#include "user_utils.h"
#include "page.h"
#include "widgets_line.h"
#include "navigation_bar.h"

#define MENU_ITEM_HEIGHT 110

lv_obj_t *CreateUserMenu(lv_obj_t *parent, const UserMenuItem_t *items, uint32_t itemCount)
{
    CreateGeneralNavigationBar(parent);
    lv_obj_t *bg = lv_obj_create(parent);
    lv_obj_set_style_bg_color(bg, lv_color_make(0x40, 0x40, 0x40), 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 32, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_set_size(bg, MY_DISP_HOR_RES - 20, MY_DISP_VER_RES - STATUS_BAR_HEIGHT - NAVIGATION_BAR_HEIGHT - 20);
    lv_obj_align(bg, LV_ALIGN_TOP_MID, 0, NAVIGATION_BAR_HEIGHT + 10);
    lv_obj_set_style_clip_corner(bg, true, 0);
    lv_obj_set_scrollbar_mode(bg, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *line = CreateLine(bg, 260);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
    lv_obj_set_style_bg_color(line, lv_color_black(), 0);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 1);
    for (uint32_t i = 0; i < itemCount; i++) {
        lv_obj_t *btn = lv_btn_create(bg);
        lv_obj_set_size(btn, MY_DISP_HOR_RES - 20, MENU_ITEM_HEIGHT - 6);
        lv_obj_set_style_radius(btn, 0, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, i * MENU_ITEM_HEIGHT + 6);
        lv_obj_set_style_bg_color(btn, lv_color_make(0x40, 0x40, 0x40), 0);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, items[i].text);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_event_cb(btn, items[i].handler, LV_EVENT_CLICKED, NULL);
        line = CreateLine(bg, MY_DISP_HOR_RES - 20);
        lv_obj_set_style_line_color(line, lv_color_black(), 0);
        lv_obj_set_style_bg_color(line, lv_color_black(), 0);
        lv_obj_align(line, LV_ALIGN_TOP_MID, 0, i * MENU_ITEM_HEIGHT + MENU_ITEM_HEIGHT + 2);
    }
    return bg;
}
