#ifndef _NAVIGATION_BAR_H
#define _NAVIGATION_BAR_H

#include "stdint.h"
#include "stdbool.h"
#include "lvgl.h"

#define NAVIGATION_BAR_HEIGHT       64
typedef struct {
    const void *leftImgSrc;
    lv_event_cb_t leftBtnCb;
    const void *rightImgSrc;
    lv_event_cb_t rightBtnCb;
    const char *middleText;
} NavigationBar_t;

void CreateNavigationBar(lv_obj_t *parent, const NavigationBar_t *navigationBar);
void CreateGeneralNavigationBar(lv_obj_t *parent, const char *title);

#endif
