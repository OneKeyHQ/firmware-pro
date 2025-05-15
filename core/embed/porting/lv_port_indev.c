/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "stdio.h"
//#include "touch_task.h"
#include "user_utils.h"


static void touchpad_init(void);
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);


/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;


void lv_port_indev_init(void)
{
    touchpad_init();

    /*Register a touchpad input device*/
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read);

}

/*Initialize your touchpad*/
static void touchpad_init(void)
{
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data)
{
    UNUSED(indev_drv);
    //TouchStatus_t *pTouchStatus = GetTouchStatus();
    //data->state = pTouchStatus->touch ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    //data->continue_reading = pTouchStatus->continueReading;
    //data->point.x = pTouchStatus->x;
    //data->point.y = pTouchStatus->y;
}

