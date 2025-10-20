#ifndef _FP_SENSOR_HAL_H_
#define _FP_SENSOR_HAL_H_

#include <stdbool.h>
#include <stdint.h>

#include STM32_HAL_H

// pin defines
#define FP_CS_PORT    GPIOA
#define FP_CS_PIN     GPIO_PIN_15

#define FP_RESET_PORT GPIOB
#define FP_RESET_PIN  GPIO_PIN_14

#define FP_IRQ_PORT   GPIOB
#define FP_IRQ_PIN    GPIO_PIN_15

#define FP_EXTI_PORT  EXTI_GPIOB
#define FP_EXTI_PIN   EXTI_LINE_15

typedef struct
{
    uint32_t port;
    uint32_t pin;
} fp_sensor_pin_pair_t;

typedef struct
{
    void (*delay_us)(uint32_t us);
    void (*delay_ms)(uint32_t ms);

    // gpio allows read or write a pin status
    void (*gpio_ctrl)(bool enable);
    void (*gpio_chip_select)(bool activate);
    void (*gpio_reset)(bool activate);
    void (*gpio_irq_ctrl)(bool activate);
    bool (*gpio_irq_status)(void);

    // seq will include delays while using gpio to complete the operaction
    uint32_t seq_delayMs_chipRst;
    void (*seq_chip_reset)(void);
    uint32_t seq_delayMs_chipSel;
    void (*seq_chip_select)(bool activate);

    void (*bus_ctrl)(bool enable);
    void (*bus_read)(uint8_t* buf, uint16_t size);
    void (*bus_write)(uint8_t* buf, uint16_t size);
    void (*bus_rw)(uint8_t* buf_w, uint16_t len_w, uint8_t* buf_r, uint16_t len_r);

    uint32_t data_sector_size;
    bool (*data_init)(bool as_new);
    bool (*data_read)(uint32_t offset, uint8_t* buf, uint32_t size);
    bool (*data_write)(uint32_t offset, uint8_t* buf, uint32_t size);
    bool (*data_syncToSE)(void);

    bool (*data_slot_get)(uint8_t id, uint8_t* buf, uint32_t size);
    bool (*data_slot_set)(uint8_t id, uint8_t* buf, uint32_t size);
    bool (*data_slot_clear)(uint8_t id);
    bool (*data_slot_syncToSE)(uint8_t id);
} fp_sensor_ll_interface_t;

fp_sensor_ll_interface_t* fp_senrsor_get_LLIF(uint32_t delayMs_chipRst, uint32_t delayMs_chipSel);

#endif //_FP_SENSOR_HAL_H_
