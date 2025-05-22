#include "page.h"
#include "stdio.h"
#include "lvgl.h"
#include "pages_declare.h"
#include "ui_msg.h"
#include "user_utils.h"
#include "navigation_bar.h"
#include "status_bar.h"
#include "images_declare.h"

static void AboutPageInit(void);

Page_t g_aboutPage = {
    .init = AboutPageInit,
};

#define LV_COLOR_GRAY() lv_color_make(0x90, 0x90, 0x90)
#define TITLE_GAP 50
#define CONTENT_GAP 30

static void AboutPageInit(void)
{
    uint32_t y = 20;
    CreateGeneralNavigationBar(GetPageBackground(), "About Device");
    lv_obj_t *aboutContainer = lv_obj_create(GetPageBackground());
    //lv_obj_remove_style_all(aboutContainer);
    lv_obj_set_style_bg_color(aboutContainer, lv_color_make(0x20, 0x20, 0x20), 0);
    lv_obj_set_style_radius(aboutContainer, 40, 0);
    lv_obj_set_style_border_width(aboutContainer, 0, 0);
    lv_obj_set_style_pad_all(aboutContainer, 0, 0);
    lv_obj_set_size(aboutContainer, MY_DISP_HOR_RES - 40, LV_SIZE_CONTENT);
    //lv_obj_set_layout(aboutContainer, LV_LAYOUT_FLEX);
    //lv_obj_set_style_flex_flow(aboutContainer, LV_FLEX_FLOW_COLUMN, 0);
    lv_obj_align(aboutContainer, LV_ALIGN_TOP_MID, 0, NAVIGATION_BAR_HEIGHT + 40);

    lv_obj_t *lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "Model");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "OneKey Pro");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "Bluetooth Name");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "Pro OFE7");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "System");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "4.13.0[73c30e6-65d8e74]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "Bluetooth");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "2.3.3 [82f4ec8-41e681a]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "Bootloader");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "2.7.0 [16143e6-f31b4a0]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "Boardloader");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "1.6.2");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "SE Firmware");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O1: 1.1.5 [dfc6ccf-02b4a4e]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O2: 1.1.3[c85f4b4-ad3de46]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O3: 1.1.3[c85f4b4-67c9876]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O4: 1.1.3[c85f4b4-38d5675]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "SE Bootloader");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O1: 1.0.1[c85f4b4-5c1466b]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O2: 1.0.1[c85f4b4-21d29a8]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O3: 1.0.1[c85f4b4-0abcee61");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "O4: 1.0.1[c85f4b4-ee58784]");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "Serial Number");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "PRB42B0186A");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    y += TITLE_GAP;
    lable = lv_label_create(aboutContainer);
    lv_label_set_text(lable, "FCC ID");
    lv_obj_set_style_text_color(lable, LV_COLOR_GRAY(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);
    y += CONTENT_GAP;
    lable = lv_label_create(aboutContainer);
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_label_set_text(lable, "2BB8VP1");
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_TOP_LEFT, 20, y);

    lv_obj_t *img = lv_img_create(aboutContainer);
    lv_img_set_src(img, &img_fcc_logo);
    lv_obj_align(img, LV_ALIGN_TOP_RIGHT, -20, y);

    lv_obj_t *button = lv_button_create(GetPageBackground());
    lv_obj_set_style_outline_width(button, 0, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);

    lv_obj_set_style_bg_color(button, lv_color_make(0x20, 0x20, 0x20), 0);
    lv_obj_set_style_radius(button, 40, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_set_size(button, MY_DISP_HOR_RES - 40, 120);
    lv_obj_align_to(button, aboutContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lable = lv_label_create(button);
    lv_label_set_text(lable, "System Update");
    lv_obj_set_style_text_color(lable, lv_color_white(), 0);
    lv_obj_set_style_text_font(lable, &lv_font_montserrat_30, 0);
    lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);
}
