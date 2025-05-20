#include "page.h"
#include "user_assert.h"
#include "status_bar.h"


Page_t *g_currentPage = NULL;


static void ShowCurrentPage(void);


void EnterNewPage(Page_t *page)
{
    if (g_currentPage != NULL) {
        if (g_currentPage->deinit != NULL) {
            g_currentPage->deinit();
        }
        lv_obj_delete(g_currentPage->background);
        g_currentPage->active = false;
    }
    page->previous = g_currentPage;
    g_currentPage = page;
    ShowCurrentPage();
}


void EnterPreviousPage(void)
{
    ASSERT(g_currentPage != NULL);
    ASSERT(g_currentPage->previous != NULL);
    if (g_currentPage->deinit != NULL) {
        g_currentPage->deinit();
    }
    lv_obj_delete(g_currentPage->background);
    g_currentPage->active = false;
    g_currentPage = g_currentPage->previous;
    ShowCurrentPage();
}


void BackToRootPage(void)
{
    ASSERT(g_currentPage != NULL);
    while (g_currentPage->previous != NULL) {
        EnterPreviousPage();
    }
}


lv_obj_t *GetPageBackground(void)
{
    return g_currentPage->background;
}


void HandleCurrentPageMsg(uint32_t code, void *data, uint32_t dataLen)
{
    if (g_currentPage->msgHandler) {
        g_currentPage->msgHandler(code, data, dataLen);
    }
}

Page_t *GetCurrentPage(void)
{
    return g_currentPage;
}

static void ShowCurrentPage(void)
{
    g_currentPage->background = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(g_currentPage->background, lv_color_black(), 0);
    lv_obj_set_style_border_width(g_currentPage->background, 0, 0);
    lv_obj_set_style_radius(g_currentPage->background, 0, 0);
    if (g_currentPage->fullScreen) {
        lv_obj_set_size(g_currentPage->background, MY_DISP_HOR_RES, MY_DISP_VER_RES);
        lv_obj_align(g_currentPage->background, LV_ALIGN_TOP_LEFT, 0, 0);
    } else {
        lv_obj_set_size(g_currentPage->background, MY_DISP_HOR_RES, MY_DISP_VER_RES - STATUS_BAR_HEIGHT);
        lv_obj_align(g_currentPage->background, LV_ALIGN_TOP_LEFT, 0, STATUS_BAR_HEIGHT);
    }

    g_currentPage->init();
    g_currentPage->active = true;
}
