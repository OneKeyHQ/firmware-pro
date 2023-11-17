#ifndef _PN532_INTERFACE_
#define _PN532_INTERFACE_

#include <stdint.h>
#include <stdbool.h>

#include "pn532_defines.h"

typedef struct _PN532_INTERFACE
{
    bool (*bus_init)(void);
    bool (*bus_deinit)(void);
    void (*reset_ctl)(bool);
    void (*chip_sel_ctl)(bool);
    bool (*irq_read)(void);
    bool (*rw)(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count);
    void (*delay_ms)(uint16_t timeout);
    void (*log)(const char* fmt, ...);
} PN532_INTERFACE;

extern PN532_INTERFACE* pn532_interface;
bool PN532_InterfaceSetup(PN532_INTERFACE* interface);

#endif //_PN532_INTERFACE_