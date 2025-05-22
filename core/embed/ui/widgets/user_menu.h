
#ifndef _USER_MENU_H
#define _USER_MENU_H

#include "stdint.h"
#include "stdbool.h"
#include "lvgl.h"

typedef struct {
    const char *text;
    lv_event_cb_t handler;
} UserMenuItem_t;

lv_obj_t *CreateUserMenu(lv_obj_t *parent, const UserMenuItem_t *items, uint32_t itemCount);


#endif
