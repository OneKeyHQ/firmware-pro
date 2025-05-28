/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "cmsis_os2.h"
#include "sdram.h"
#include "mipi_lcd.h"
#include "stm32h7xx_hal.h"
#include "stdio.h"

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
#warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
#define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
#warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
#define MY_DISP_VER_RES    240
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#define LVGL_GRAM_ADDRESS (FMC_SDRAM_LTDC_BUFFER_ADDRESS + 800 * 480 * 2)

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    /* Example 1
     * One buffer for partial rendering*/
    /*full screen buffer*/
#ifdef LVGL_DOUBLE_BUFFER
    lv_display_set_buffers(disp, (void *)LVGL_GRAM_ADDRESS, (void *)(LVGL_GRAM_ADDRESS + MY_DISP_HOR_RES * MY_DISP_VER_RES * 2), MY_DISP_HOR_RES * MY_DISP_VER_RES * 2, LV_DISPLAY_RENDER_MODE_FULL);
#else
    lv_display_set_buffers(disp, (void *)LVGL_GRAM_ADDRESS, NULL, MY_DISP_HOR_RES * MY_DISP_VER_RES * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif
    GPIO_InitTypeDef gpio_init_structure = {0};
    gpio_init_structure.Mode = GPIO_MODE_INPUT;
    gpio_init_structure.Pull = GPIO_NOPULL;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init_structure.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOJ, &gpio_init_structure);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static uint32_t frameCount = 0;
static uint32_t lastTime = 0;

static void FrameMonitorTask(void)
{
    frameCount++;

    uint32_t now = lv_tick_get();
    if (now - lastTime >= 1000) {
        printf("FPS: %lu\n", frameCount);
        frameCount = 0;
        lastTime = now;
    }
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if (disp_flush_enabled) {
#ifdef LVGL_DOUBLE_BUFFER
        while (HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_2) == GPIO_PIN_RESET) {
        }
        lcd_set_src_addr((uint32_t)px_map);
#else
        //lv_draw_sw_rgb565_swap(px_map, (area->y2 - area->y1 + 1) * (area->x2 - area->x1 + 1));
        dma2d_copy_buffer((uint32_t *)px_map, (uint32_t *)lcd_get_src_addr(),
                  area->x1, area->y1, area->x2 - area->x1 + 1,
                  area->y2 - area->y1 + 1);
#endif
        FrameMonitorTask();
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_display_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
