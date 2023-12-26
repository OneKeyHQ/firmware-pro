#ifndef _TREZORHAL_THD89_H_
#define _TREZORHAL_THD89_H_

#include "secbool.h"

#define THD89_MASTER_ADDRESS (0x10 << 1)
#define THD89_FINGER_ADDRESS (0x13 << 1)

void thd89_io_init(void);
void thd89_init(void);
void thd89_power_up(bool up);
void thd89_reset(void);
secbool thd89_transmit(uint8_t *cmd, uint16_t len, uint8_t *resp,
                       uint16_t *resp_len);
secbool thd89_fp_transmit(uint8_t *cmd, uint16_t len, uint8_t *resp,
                          uint16_t *resp_len);
secbool thd89_transmit_ex(uint8_t addr, uint8_t *cmd, uint16_t len,
                          uint8_t *resp, uint16_t *resp_len);
uint16_t thd89_last_error();

#endif
