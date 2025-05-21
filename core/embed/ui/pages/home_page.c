#include "page.h"
#include "stdio.h"
#include "lvgl.h"
#include "pages_declare.h"
#include "ui_msg.h"
#include "navigation_bar.h"
#include "user_utils.h"
#include "user_memory.h"
#include "status_bar.h"
#include "images_declare.h"


static void HomePageInit(void);
static void HomePageDeinit(void);
static void HomePageMsgHandler(uint32_t code, void *data, uint32_t dataLen);
static void ImgBtnEventHandler(lv_event_t *e);

Page_t g_homePage = {
    .init = HomePageInit,
    .deinit = HomePageDeinit,
    .msgHandler = HomePageMsgHandler,
    .fullScreen = true,
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
    lv_obj_t *mainTileView, *outerTileView, *tile;
    lv_obj_t *img, *label;

    //outer tile
    outerTileView = lv_tileview_create(GetPageBackground());
    lv_obj_set_style_bg_color(outerTileView, lv_color_black(), 0);
    lv_obj_set_style_border_width(outerTileView, 0, 0);
    lv_obj_set_scrollbar_mode(outerTileView, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_size(outerTileView, 480, 800);
    lv_timer_handler();
    printf("outerTileView size: %ld x %ld\n", lv_obj_get_width(outerTileView), lv_obj_get_height(outerTileView));
    printf("GetPageBackground size: %ld x %ld\n", lv_obj_get_width(GetPageBackground()), lv_obj_get_height(GetPageBackground()));

    //add wallpaper
    tile = lv_tileview_add_tile(outerTileView, 0, 0, LV_DIR_BOTTOM);
    printf("tile size: %ld x %ld\n", lv_obj_get_width(tile), lv_obj_get_height(tile));
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_set_scrollbar_mode(tile, LV_SCROLLBAR_MODE_OFF);
    img = lv_image_create(tile);
    lv_image_set_src(img, &img_wallpaper_1);
    lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);
    printf("tile size: %ld x %ld\n", lv_obj_get_width(tile), lv_obj_get_height(tile));

    //add main tile
    tile = lv_tileview_add_tile(outerTileView, 0, 1, LV_DIR_TOP);
    mainTileView = lv_tileview_create(tile);
    lv_obj_set_style_bg_color(mainTileView, lv_color_black(), 0);
    lv_obj_set_style_border_width(mainTileView, 0, 0);
    lv_obj_set_scrollbar_mode(mainTileView, LV_SCROLLBAR_MODE_OFF);
    //lv_obj_set_size(mainTileView, 480, 800);

    tile = NULL;

    for (uint32_t i = 0; i < sizeof(homePageIconList) / sizeof(HomePageIconItem_t); i++) {
        if (i % 4 == 0) {
            tile = lv_tileview_add_tile(mainTileView, i / 4, 0, LV_DIR_HOR);
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
