#ifndef _UI_MSG_H
#define _UI_MSG_H

#include "stdint.h"
#include "stdbool.h"
#include "ui_msg_code.h"

void SendUiMsg(uint32_t code, const void *data, uint32_t dataLen);

#endif
