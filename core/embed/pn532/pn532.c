// stdlib
#include <stdio.h>
#include <stdlib.h>
// pn532 lib
#include "pn532_defines.h"
#include "pn532_interface.h"
#include "pn532_hal.h"
#include "pn532_frame.h"
#include "pn532_functions.h"
// own header
#include "pn532.h"

static PN532 _pn532;
PN532* pn532 = NULL;

void PN532_LibrarySetup()
{
    pn532 = &_pn532;

    pn532->PowerOn = power_on;
    pn532->PowerOff = power_off;

    // Miscellaneous
    pn532->Diagnose = Diagnose;
    pn532->GetFirmwareVersion = GetFirmwareVersion;
    pn532->SAMConfiguration = SAMConfiguration;
    pn532->InListPassiveTarget = InListPassiveTarget;
    pn532->InDataExchange = InDataExchange;
}