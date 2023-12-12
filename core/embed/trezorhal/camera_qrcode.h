#ifndef __CAMERA_QRCODE_H__
#define __CAMERA_QRCODE_H__

#include <stdint.h>

int camera_qr_decode(uint32_t x, uint32_t y, uint8_t* data, uint32_t data_len);
void camera_qr_test(void);

#endif // __CAMERA_QRCODE_H__
