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

#define SLIDE_GRADIEMT      0
#define SLIDE_TILEVIEW      1

#define SLIDE_SELECT    SLIDE_GRADIEMT


static void HomePageInit(void);
static void HomePageDeinit(void);
static void HomePageMsgHandler(uint32_t code, void *data, uint32_t dataLen);
static void ImgBtnEventHandler(lv_event_t *e);
#if (SLIDE_SELECT == SLIDE_GRADIEMT)
static void WallpaperSlideEventHandler(lv_event_t *e);
static void MainTileViewSlideEventHandler(lv_event_t *e);
static void HideWallpaperAfterFade(lv_anim_t *a);
static void ShowMainTileView(void);
static void ShowWallpaper(void);
#endif

Page_t g_homePage = {
    .init = HomePageInit,
    .deinit = HomePageDeinit,
    .msgHandler = HomePageMsgHandler,
    .fullScreen = true,
};

typedef struct {
    const lv_image_dsc_t *imgSrc;
    const char *text;
    Page_t *page;
} HomePageIconItem_t;

static const HomePageIconItem_t homePageIconList[] = {
    {&img_app_connect, "connect", NULL},
    {&img_app_scan, "scan", NULL},
    {&img_app_address, "address", NULL},
    {&img_app_settings, "settings", &g_settingsPage},
    {&img_app_keys, "keys", NULL},
    {&img_app_backup, "backup", NULL},
    {&img_app_nft, "nft", NULL},
    {&img_app_tips, "tips", NULL},
};

lv_obj_t *g_wallpaper = NULL;
lv_obj_t *g_mainTileView = NULL;

#if (SLIDE_SELECT == SLIDE_GRADIEMT)

static void HomePageInit(void)
{
    lv_obj_t *tile;
    lv_obj_t *img, *label;

    g_wallpaper = lv_image_create(GetPageBackground());
    lv_image_set_src(g_wallpaper, &img_wallpaper_1);
    lv_obj_align(g_wallpaper, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_flag(g_wallpaper, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_wallpaper, WallpaperSlideEventHandler, LV_EVENT_CLICKED, NULL);

    //add main tile
    g_mainTileView = lv_tileview_create(GetPageBackground());
    lv_obj_set_style_bg_color(g_mainTileView, lv_color_black(), 0);
    lv_obj_set_style_border_width(g_mainTileView, 0, 0);
    lv_obj_set_scrollbar_mode(g_mainTileView, LV_SCROLLBAR_MODE_OFF);

    tile = NULL;

    for (uint32_t i = 0; i < sizeof(homePageIconList) / sizeof(HomePageIconItem_t); i++) {
        if (i % 4 == 0) {
            tile = lv_tileview_add_tile(g_mainTileView, i / 4, 0, LV_DIR_HOR);
            lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(tile, MainTileViewSlideEventHandler, LV_EVENT_CLICKED, NULL);
        }
        img = lv_image_create(tile);
        lv_image_set_src(img, homePageIconList[i].imgSrc);
        lv_obj_align(img, LV_ALIGN_CENTER, (i % 2 == 0 ? -116 : 116), ((i % 4) < 2 ? -136 : 136));
        lv_obj_set_style_image_opa(img, LV_OPA_30, LV_STATE_PRESSED);
        lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(img, ImgBtnEventHandler, LV_EVENT_CLICKED, homePageIconList[i].page);
        label = lv_label_create(tile);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
        lv_label_set_text(label, homePageIconList[i].text);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, (i % 2 == 0 ? -116 : 116), ((i % 4) < 2 ? -136 : 136) + 120);
    }
    lv_obj_add_flag(g_mainTileView, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(g_wallpaper);
}

#else

static void HomePageInit(void)
{
    lv_obj_t *mainTileView, *outerTileView, *tile;
    lv_obj_t *img, *label;

    //outer tile
    outerTileView = lv_tileview_create(GetPageBackground());
    lv_obj_set_style_bg_color(outerTileView, lv_color_black(), 0);
    lv_obj_set_style_border_width(outerTileView, 0, 0);
    lv_obj_set_scrollbar_mode(outerTileView, LV_SCROLLBAR_MODE_OFF);

    //add wallpaper
    tile = lv_tileview_add_tile(outerTileView, 0, 0, LV_DIR_BOTTOM);
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_set_scrollbar_mode(tile, LV_SCROLLBAR_MODE_OFF);
    img = lv_image_create(tile);
    lv_image_set_src(img, &img_wallpaper_1);
    lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);

    //add main tile
    tile = lv_tileview_add_tile(outerTileView, 0, 1, LV_DIR_TOP);
    mainTileView = lv_tileview_create(tile);
    lv_obj_set_style_bg_color(mainTileView, lv_color_black(), 0);
    lv_obj_set_style_border_width(mainTileView, 0, 0);
    lv_obj_set_scrollbar_mode(mainTileView, LV_SCROLLBAR_MODE_OFF);

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
        lv_obj_add_event_cb(img, ImgBtnEventHandler, LV_EVENT_CLICKED, homePageIconList[i].page);
        label = lv_label_create(tile);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
        lv_label_set_text(label, homePageIconList[i].text);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, (i % 2 == 0 ? -116 : 116), ((i % 4) < 2 ? -136 : 136) + 120);
    }
}

#endif


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
        Page_t *page = lv_event_get_user_data(e);
        if (page != NULL) {
            EnterNewPage(page);
        }
    }
}

#if (SLIDE_SELECT == SLIDE_GRADIEMT)
static void WallpaperSlideEventHandler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_TOP) {
            printf("slide up\n");
            ShowMainTileView();
        }
    }
}

static void MainTileViewSlideEventHandler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    printf("code=%d\n", code);
    if (code == LV_EVENT_CLICKED) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_BOTTOM) {
            printf("slide down\n");
            ShowWallpaper();
        }
    }
}

static void HideWallpaperAfterFade(lv_anim_t *a)
{
    lv_obj_add_flag((lv_obj_t *)a->var, LV_OBJ_FLAG_HIDDEN);
}

static void ShowMainTileView(void)
{
    lv_anim_t a1;
    lv_anim_init(&a1);
    lv_anim_set_var(&a1, g_wallpaper);
    lv_anim_set_values(&a1, LV_OPA_COVER, LV_OPA_TRANSP); // 255 → 0
    lv_anim_set_time(&a1, 500);
    lv_anim_set_exec_cb(&a1, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a1, HideWallpaperAfterFade);
    lv_anim_start(&a1);

    lv_obj_clear_flag(g_mainTileView, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(g_mainTileView, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, g_mainTileView);
    lv_anim_set_values(&a2, LV_OPA_TRANSP, LV_OPA_COVER); // 0 → 255
    lv_anim_set_time(&a2, 500);
    lv_anim_set_exec_cb(&a2, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_path_cb(&a2, lv_anim_path_ease_out);
    lv_anim_start(&a2);
}

static void ShowWallpaper(void)
{
    lv_obj_clear_flag(g_wallpaper, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(g_wallpaper, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_anim_t a1;
    lv_anim_init(&a1);
    lv_anim_set_var(&a1, g_wallpaper);
    lv_anim_set_values(&a1, LV_OPA_TRANSP, LV_OPA_COVER); // 0 → 255
    lv_anim_set_time(&a1, 500);
    lv_anim_set_exec_cb(&a1, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_path_cb(&a1, lv_anim_path_ease_out);
    lv_anim_start(&a1);

    lv_anim_t a2;
    lv_anim_init(&a2);
    lv_anim_set_var(&a2, g_mainTileView);
    lv_anim_set_values(&a2, LV_OPA_COVER, LV_OPA_TRANSP); // 255 → 0
    lv_anim_set_time(&a2, 500);
    lv_anim_set_exec_cb(&a2, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_path_cb(&a2, lv_anim_path_ease_out);
    lv_anim_start(&a2);
}
#endif
