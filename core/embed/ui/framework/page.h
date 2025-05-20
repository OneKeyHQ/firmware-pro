#ifndef _PAGE_H
#define _PAGE_H

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "lvgl.h"

typedef void (*PageInitFunc_t)(void);
typedef void (*PageDeinitFunc_t)(void);
typedef void (*HandlePageMsgFunc_t)(uint32_t code, void *data, uint32_t dataLen);


typedef struct Page_t_ {
    bool fullScreen;
    bool active;
    struct Page_t_ *previous;
    lv_obj_t *background;
    PageInitFunc_t init;
    PageDeinitFunc_t deinit;
    HandlePageMsgFunc_t msgHandler;
} Page_t;


void EnterNewPage(Page_t *page);
void EnterPreviousPage(void);
void BackToRootPage(void);
lv_obj_t *GetPageBackground(void);
void HandleCurrentPageMsg(uint32_t code, void *data, uint32_t dataLen);
Page_t *GetCurrentPage(void);


#endif
