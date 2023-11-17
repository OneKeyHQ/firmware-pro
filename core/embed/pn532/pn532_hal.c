// stdlib
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// pn532 lib
#include "pn532_defines.h"
#include "pn532_interface.h"
// own header
#include "pn532_hal.h"

void power_on()
{
    // bus init
    pn532_interface->bus_init();

    // reset
    pn532_interface->reset_ctl(false);
    pn532_interface->delay_ms(1);
    pn532_interface->reset_ctl(true);
    pn532_interface->delay_ms(2);
}

void power_off()
{
    // hold reset down, hard power down
    pn532_interface->reset_ctl(false);

    // bus deinit
    pn532_interface->bus_deinit();
}

bool bus_rw(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count)
{
    bool result = pn532_interface->rw(w_data, w_count, r_data, r_count);

    if ( !result )
    {
        pn532_interface->log("bus_rw failed");
    }

    return result;
}

uint8_t get_status()
{
    uint8_t req = PN532_SPI_STATREAD;
    uint8_t resp;
    bus_rw(&req, 1, &resp, 1);
    return resp;
}

bool get_irq()
{
    return !pn532_interface->irq_read(); // inverted as irq is active LOW
}

bool wait_irq(uint32_t timeout_ms)
{
    uint32_t delay_ms_step = timeout_ms / 10;

    while ( !get_irq() )
    {
        // check in this way to avoid overflow
        if ( timeout_ms < delay_ms_step )
        {
            return false;
        }

        pn532_interface->delay_ms(delay_ms_step);

        timeout_ms -= delay_ms_step;
    }

    return true;
}

bool is_ready()
{
    return (get_status() & PN532_SPI_READY) == PN532_SPI_READY;
}

bool wait_ready(uint32_t timeout_ms)
{
    uint32_t delay_ms_step = timeout_ms / 10;

    while ( !is_ready() )
    {
        // check in this way to avoid overflow
        if ( timeout_ms < delay_ms_step )
        {
            // pn532_interface->log("wait_ready timeout");
            return false;
        }

        pn532_interface->delay_ms(delay_ms_step);

        timeout_ms -= delay_ms_step;
    }

    return true;
}

bool read_data(uint8_t* buff, uint16_t len)
{
    uint8_t req = PN532_SPI_DATAREAD;
    uint8_t* resp = buff;
    return bus_rw(&req, 1, resp, len);
}

bool write_data(uint8_t* buff, uint16_t len)
{
    uint8_t req[len + 1];
    req[0] = PN532_SPI_DATAWRITE;
    memcpy(&req[1], buff, len);

    uint8_t* resp = NULL;
    return bus_rw(req, len + 1, resp, 0);
}