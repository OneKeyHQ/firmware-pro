#include <string.h>

#include "memzero.h"
#include "pn532_defines.h"
#include "pn532_hal.h"

const PN532_INTERFACE* pn532_interface = NULL;

bool PN532_InterfaceSetup(const PN532_INTERFACE* interface)
{

    if ( interface == NULL )
    {
        return false;
    }

    // all functions must be filled
    if ( interface->bus_init == NULL || interface->bus_deinit == NULL || interface->reset_ctl == NULL ||
         interface->chip_sel_ctl == NULL || interface->rw == NULL || interface->delay_ms == NULL )
    {
        return false;
    }

    pn532_interface = interface;
    return true;
}

bool pn532_hal_power_on(void)
{
    // bus init
    if ( !pn532_interface->bus_init() )
    {
        return false;
    }

    // reset - PN532 needs significant boot time after reset release
    pn532_interface->reset_ctl(false);
    pn532_interface->delay_ms(100); // hold reset low 100ms
    pn532_interface->reset_ctl(true);
    pn532_interface->delay_ms(1000); // wait for PN532 firmware boot (~1000ms)

    return true;
}

void pn532_hal_power_off(void)
{
    // hold reset down, hard power down
    pn532_interface->reset_ctl(false);

    (void)pn532_interface->bus_deinit();
}

bool pn532_hal_bus_rw(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count)
{
    return pn532_interface->rw(w_data, w_count, r_data, r_count);
}

uint8_t pn532_hal_get_status(void)
{
    uint8_t req = PN532_SPI_STATREAD;
    uint8_t resp = 0; // fail-closed: 0 has PN532_SPI_READY clear
    if ( !pn532_hal_bus_rw(&req, 1, &resp, 1) )
    {
        return 0;
    }
    return resp;
}

bool pn532_hal_is_ready(void)
{
    return (pn532_hal_get_status() & PN532_SPI_READY) == PN532_SPI_READY;
}

bool pn532_hal_wait_ready(uint32_t timeout_ms)
{
    uint32_t delay_ms_step = 10;

    while ( !pn532_hal_is_ready() )
    {
        // check in this way to avoid overflow
        if ( timeout_ms < delay_ms_step )
        {
            // timeout
            return false;
        }

        pn532_interface->delay_ms(delay_ms_step);

        timeout_ms -= delay_ms_step;
    }

    return true;
}

bool pn532_hal_read_data(uint8_t* buff, uint16_t len)
{
    uint8_t req = PN532_SPI_DATAREAD;
    uint8_t* resp = buff;
    return pn532_hal_bus_rw(&req, 1, resp, len);
}

bool pn532_hal_write_data(uint8_t* buff, uint16_t len)
{
    uint8_t req[PN532_FRAME_RAW_MAX + 1];
    if ( len > PN532_FRAME_RAW_MAX )
        return false;

    req[0] = PN532_SPI_DATAWRITE;
    memcpy(&req[1], buff, len);

    bool ok = pn532_hal_bus_rw(req, len + 1, NULL, 0);
    memzero(req, sizeof(req));
    return ok;
}
