#ifndef __FPSENSOR_COMMON_H
#define __FPSENSOR_COMMON_H

#include "fpsensor_driver.h"

#define FPSENSOR_STATUS_REG_MODE_MASK                                              \
  (FPSENSOR_STATUS_REG_BIT_MAIN_IDLE_CMD | FPSENSOR_STATUS_REG_BIT_SYNC_PWR_IDLE | \
   FPSENSOR_STATUS_REG_BIT_PWR_DWN_OSC_HIN)

#define FPSENSOR_STATUS_REG_IN_SLEEP_MODE 0

typedef enum
{
    FPSENSOR_STATUS_REG_BIT_IRQ = 1 << 0,
    FPSENSOR_STATUS_REG_BIT_MAIN_IDLE_CMD = 1 << 1,
    FPSENSOR_STATUS_REG_BIT_SYNC_PWR_IDLE = 1 << 2,
    FPSENSOR_STATUS_REG_BIT_PWR_DWN_OSC_HIN = 1 << 3,
    FPSENSOR_STATUS_REG_BIT_FIFO_EMPTY = 1 << 4,
    FPSENSOR_STATUS_REG_BIT_FIFO_FULL = 1 << 5,
    FPSENSOR_STATUS_REG_BIT_MISO_EDGRE_RISE_EN = 1 << 6
} fpsensor_status_reg_t;

typedef enum
{
    FPSENSOR_IRQ_REG_BIT_FINGER_DOWN = 1 << 0,
    FPSENSOR_IRQ_REG_BIT_ERROR = 1 << 2,
    FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA = 1 << 5,
    FPSENSOR_IRQ_REG_BIT_COMMAND_DONE = 1 << 7,
    FPSENSOR_IRQ_REG_BITS_RESET = 0xff
} fpsensor_irq_reg_t;

typedef enum
{
    DISPLAY_FORWORD = 0x0b,
    DISPLAY_INVERSE = 0x03,
} fpsensor_invert_color_t;

uint8_t fpsensor_print_HWID(void);
uint8_t fpsensor_print_VID(void);
uint8_t fpsensor_set_adc(fpsensor_adc_t* adc);
uint8_t fpsensor_set_inverse(uint8_t* buffer, uint32_t length, fpsensor_invert_color_t color);
uint8_t fpsensor_set_capture_crop(
    uint8_t* buffer, uint32_t length, uint32_t rowStart, uint32_t rowCount, uint32_t colStart,
    uint32_t colGroup
);

uint8_t fpsensor_capture_image(void);
uint8_t fpsensor_soft_reset(void);
uint8_t fpsensor_get_img_data(uint8_t* buffer, uint32_t length);
uint8_t fpsensor_finger_present_status(uint8_t* buffer, uint32_t length);
uint8_t fpsensor_fpc_status(uint8_t* buffer, uint32_t length);
#endif
