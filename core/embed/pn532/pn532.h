#ifndef _PN532_
#define _PN532_

#include <stdint.h>
#include <stdbool.h>

#include "pn532_defines.h"

#include "pn532_interface.h"

typedef struct _PN532
{
    void (*PowerOn)(void);
    void (*PowerOff)(void);

    // Miscellaneous
    bool (*Diagnose)(PN532_DIAG, uint8_t*, uint8_t);
    bool (*GetFirmwareVersion)(PN532_FW_VER*);
    bool (*SAMConfiguration)(PN532_SAM_MODE, uint8_t, bool);
    bool (*InListPassiveTarget
    )(PN532_InListPassiveTarget_Params params, PN532_InListPassiveTarget_Results* results);
    bool (*InDataExchange
    )(uint8_t Tg, uint8_t* DataOut, uint16_t DataOut_len, uint8_t* Status, uint8_t* DataIn,
      uint16_t* DataIn_len);
} PN532;

extern PN532* pn532;
void PN532_LibrarySetup();

#endif //_PN532_