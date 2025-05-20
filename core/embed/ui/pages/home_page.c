#include "page.h"
#include "stdio.h"
#include "lvgl.h"
#include "pages_declare.h"
#include "ui_msg.h"
#include "navigation_bar.h"
#include "user_utils.h"
#include "user_memory.h"
#include "images_declare.h"


static void HomePageInit(void);
static void HomePageDeinit(void);
static void HomePageMsgHandler(uint32_t code, void *data, uint32_t dataLen);
static void ImgBtnEventHandler(lv_event_t *e);

Page_t g_homePage = {
    .init = HomePageInit,
    .deinit = HomePageDeinit,
    .msgHandler = HomePageMsgHandler,
};

typedef struct {
    const lv_image_dsc_t *imgSrc;
    const char *text;
} HomePageIconItem_t;

static const HomePageIconItem_t homePageIconList[] = {
    {&img_app_connect, "connect"},
    {&img_app_scan, "scan"},
    {&img_app_address, "address"},
    {&img_app_settings, "settings"},
    {&img_app_keys, "keys"},
    {&img_app_backup, "backup"},
    {&img_app_nft, "nft"},
    {&img_app_tips, "tips"},
};

static void HomePageInit(void)
{
    lv_obj_t *tileView, *tile;
    lv_obj_t *img, *label;

    tileView = lv_tileview_create(GetPageBackground());
    lv_obj_set_style_bg_color(tileView, lv_color_black(), 0);
    lv_obj_set_style_border_width(tileView, 0, 0);
    tile = NULL;

    for (uint32_t i = 0; i < sizeof(homePageIconList) / sizeof(HomePageIconItem_t); i++) {
        if (i % 4 == 0) {
            tile = lv_tileview_add_tile(tileView, i / 4, 0, LV_DIR_HOR);
        }
        img = lv_image_create(tile);
        lv_image_set_src(img, homePageIconList[i].imgSrc);
        lv_obj_align(img, LV_ALIGN_CENTER, (i % 2 == 0 ? -116 : 116), ((i % 4) < 2 ? -136 : 136));
        lv_obj_set_style_image_opa(img, LV_OPA_30, LV_STATE_PRESSED);
        lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(img, ImgBtnEventHandler, LV_EVENT_CLICKED, NULL);
        label = lv_label_create(tile);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
        lv_label_set_text(label, homePageIconList[i].text);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, (i % 2 == 0 ? -116 : 116), ((i % 4) < 2 ? -136 : 136) + 120);
    }
}


static void HomePageDeinit(void)
{
}

static void HomePageMsgHandler(uint32_t code, void *data, uint32_t dataLen)
{
    UNUSED(data);
    UNUSED(dataLen);
}

static void ImgBtnEventHandler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        printf("clinked\n");
    }
}
