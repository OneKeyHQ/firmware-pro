
#include "ui_msg.h"
#include "stdio.h"
//#include "user_msg.h"
#include "user_assert.h"

void SendUiMsg(uint32_t code, const void *data, uint32_t dataLen)
{
    if (data == NULL) {
        //PubValueMsg(UI_MSG_USER_EVENT, code);
    } else {
        ASSERT(dataLen > 0);
        //PubValueBufferMsg(UI_MSG_USER_EVENT, code, data, dataLen);
    }
}
