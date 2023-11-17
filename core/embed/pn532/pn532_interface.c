// stdlib
#include <stdio.h>
#include <stdlib.h>
// pn532 lib
#include "pn532_defines.h"
// own header
#include "pn532_interface.h"

PN532_INTERFACE* pn532_interface = NULL;

bool PN532_InterfaceSetup(PN532_INTERFACE* interface)
{

    if ( interface == NULL )
    {
        return false;
    }

    // all functions must be filled
    if ( interface->bus_init == NULL || interface->bus_deinit == NULL || interface->reset_ctl == NULL ||
         interface->chip_sel_ctl == NULL || interface->irq_read == NULL || interface->rw == NULL ||
         interface->delay_ms == NULL || interface->log == NULL )
    {
        return false;
    }

    pn532_interface = interface;
    return true;
}