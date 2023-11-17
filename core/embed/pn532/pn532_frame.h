#ifndef _PN532_FRAME_
#define _PN532_FRAME_

#include <stdint.h>
#include <stdbool.h>

#include "pn532_defines.h"

PN532_FRAME* PackFrame(PN532_FRAME_TYPE type, uint8_t* buff, uint8_t len);
void DiscardFrame(PN532_FRAME* frame);
bool WriteFrame(PN532_FRAME* frame);
bool ReadFrame(PN532_FRAME** frame);

#endif // _PN532_FRAME_