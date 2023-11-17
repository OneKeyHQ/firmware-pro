#ifndef _PN532_HAL_
#define _PN532_HAL_

#include <stdint.h>
#include <stdbool.h>

void power_on();
void power_off();

bool bus_rw(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count);

uint8_t get_status();
bool get_irq();

bool is_ready();
bool wait_ready(uint32_t timeout_ms);

bool hal_frame_wrapper_spi(uint8_t* buff, uint8_t buff_len, uint8_t data_len, PN532_FRAME_TYPE_SPI type);

bool read_data(uint8_t* buff, uint16_t len);
bool write_data(uint8_t* buff, uint16_t len);

#endif // _PN532_HAL_