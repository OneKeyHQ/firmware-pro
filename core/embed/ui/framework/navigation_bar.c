#include "navigation_bar.h"
#include "images_declare.h"
#include "page.h"

static void BackButtonHandler(lv_event_t *e);

void CreateNavigationBar(lv_obj_t *parent, const NavigationBar_t *navigationBar)
{
    lv_obj_t *img, *btn, *label;
    lv_obj_t *bg = lv_obj_create(parent);
    lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_set_size(bg, MY_DISP_HOR_RES, NAVIGATION_BAR_HEIGHT);
    if (navigationBar->leftImgSrc != NULL) {
        btn = lv_btn_create(bg);
        lv_obj_set_size(btn, 64, 64);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, navigationBar->leftBtnCb, LV_EVENT_CLICKED, NULL);
        img = lv_img_create(btn);
        lv_img_set_src(img, navigationBar->leftImgSrc);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    }
    if (navigationBar->rightImgSrc != NULL) {
        btn = lv_btn_create(bg);
        lv_obj_set_size(btn, 64, 64);
        lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, navigationBar->rightBtnCb, LV_EVENT_CLICKED, NULL);
        img = lv_img_create(btn);
        lv_img_set_src(img, navigationBar->rightImgSrc);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    }
    if (navigationBar->middleText != NULL) {
        label = lv_label_create(bg);
        lv_label_set_text(label, navigationBar->middleText);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }
}

void CreateGeneralNavigationBar(lv_obj_t *parent, const char *title)
{
    NavigationBar_t navigationBar = {
        .leftImgSrc = &img_nav_back,
        .leftBtnCb = BackButtonHandler,
        .rightImgSrc = NULL,
        .rightBtnCb = NULL,
        .middleText = title,
    };
    CreateNavigationBar(parent, &navigationBar);
}

static void BackButtonHandler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        EnterPreviousPage();
    }
}

