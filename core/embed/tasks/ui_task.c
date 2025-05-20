#include "ui_task.h"
#include "cmsis_os2.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "user_utils.h"
#include "demos/lv_demos.h"
#include "page.h"
#include "pages_declare.h"

#define LVGL_TICK                       2


osThreadId_t g_uiTaskHandle;
osTimerId_t g_lvglTickTimer;

static void UiTask(void *argument);
static void LvglTickTimerFunc(void *argument);


void CreateUiTask(void)
{
    const osThreadAttr_t testtTask_attributes = {
        .name = "UiTask",
        .stack_size = 1024 * 16,
        .priority = (osPriority_t) osPriorityHigh,
    };
    g_uiTaskHandle = osThreadNew(UiTask, NULL, &testtTask_attributes);
    g_lvglTickTimer = osTimerNew(LvglTickTimerFunc, osTimerPeriodic, NULL, NULL);
    osTimerStart(g_lvglTickTimer, LVGL_TICK);
}

static void UiTask(void *argument)
{
    UNUSED(argument);

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    //lv_demo_widgets();
    EnterNewPage(&g_homePage);

    while (1) {
        lv_timer_handler();
        osDelay(1);
    }
}


static void LvglTickTimerFunc(void *argument)
{
    UNUSED(argument);
    lv_tick_inc(LVGL_TICK);
}

