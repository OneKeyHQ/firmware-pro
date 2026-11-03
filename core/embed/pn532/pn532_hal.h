#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "pn532_defines.h"

typedef struct _PN532_INTERFACE
{
    bool (*bus_init)(void);
    bool (*bus_deinit)(void);
    void (*reset_ctl)(bool);
    void (*chip_sel_ctl)(bool);
    bool (*rw)(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count);
    void (*delay_ms)(uint16_t timeout);
} PN532_INTERFACE;

extern const PN532_INTERFACE* pn532_interface;
bool PN532_InterfaceSetup(const PN532_INTERFACE* interface);

bool pn532_hal_power_on(void);
void pn532_hal_power_off(void);

bool pn532_hal_bus_rw(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count);

uint8_t pn532_hal_get_status(void);
bool pn532_hal_is_ready(void);
bool pn532_hal_wait_ready(uint32_t timeout_ms);

bool pn532_hal_read_data(uint8_t* buff, uint16_t len);
bool pn532_hal_write_data(uint8_t* buff, uint16_t len);
