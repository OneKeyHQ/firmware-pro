#pragma once

#include "pn532_defines.h"

PN532_FRAME* pn532_frame_pack(PN532_FRAME_TYPE type, uint8_t* buff, uint16_t len);
void pn532_frame_discard(PN532_FRAME* frame);
bool pn532_frame_write(PN532_FRAME* frame);
bool pn532_frame_read(PN532_FRAME** frame);
