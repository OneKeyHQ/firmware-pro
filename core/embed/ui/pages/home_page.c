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


Page_t g_homePage = {
    .init = HomePageInit,
    .deinit = HomePageDeinit,
    .msgHandler = HomePageMsgHandler,
};


static void HomePageInit(void)
{
    //lv_obj_t *lable = lv_label_create(GetPageBackground());
    //lv_label_set_text(lable, "hello");
    //lv_obj_align(lable, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *img = lv_image_create(GetPageBackground());
    lv_image_set_src(img, &img_app_connect);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
}


static void HomePageDeinit(void)
{
}

static void HomePageMsgHandler(uint32_t code, void *data, uint32_t dataLen)
{
    UNUSED(data);
    UNUSED(dataLen);
}
