#include "page.h"
#include "stdio.h"
#include "lvgl.h"
#include "pages_declare.h"
#include "ui_msg.h"
#include "user_utils.h"
#include "navigation_bar.h"
#include "user_menu.h"

static void SettingsPageInit(void);
static void AbountButtonEventHandler(lv_event_t *e);

Page_t g_settingsPage = {
    .init = SettingsPageInit,
};

static void SettingsPageInit(void)
{
    UserMenuItem_t items[] = {
        {"Settings 1", NULL},
        {"Settings 2", NULL},
        {"Settings 3", NULL},
        {"Settings 4", NULL},
        {"Settings 5", NULL},
        {"About", AbountButtonEventHandler},
        {"Settings 7", NULL},
        {"Settings 8", NULL},
        {"Settings 9", NULL},
    };
    CreateGeneralNavigationBar(GetPageBackground(), NULL);
    CreateUserMenu(GetPageBackground(), items, 9);
}

static void AbountButtonEventHandler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        printf("about\n");
        EnterNewPage(&g_aboutPage);
    }
}