#ifndef _LITE_CARD_H_
#define _LITE_CARD_H_

#include <stdbool.h>
#include <stdint.h>

#define CARD_AID_MAX_LEN 32

bool lite_card_select_aid(uint8_t* aid, uint8_t aid_len);
bool lite_card_data_exchange_test(void);
bool lite_card_apdu(uint8_t* apdu, uint16_t apdu_len, uint8_t* response,
                    uint16_t* response_len, uint8_t* sw1sw2, bool safe);
#endif
