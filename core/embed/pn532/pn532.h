#ifndef _PN532_H_
#define _PN532_H_

#include "pn532_spi.h"
#include "pn532_stub.h"
#include "pn532_defines.h"

#define PN532_FRAME_DATA_MAX_LENGTH            255
#define PN532_TIMEOUT_MS_NORMAL                200
#define PN532_TIMEOUT_MS_DATA_EXCHANGE         1800
#define PN532_TIMEOUT_MS_NORMAL_PASSSIVETARGET 30

typedef struct
{
    void (*delay_ms)(uint32_t timeout);
    pn532_stub_t* stub_controller;
    pn532_spi_t* spi_controller;
} pn532_controller_t;

typedef union
{
    struct __attribute__((packed))
    {
        uint8_t preamble;
        uint8_t start_code1;
        uint8_t start_code2;
        uint8_t len;
        uint8_t lcs;
        uint8_t tfi;
        uint8_t data[PN532_FRAME_DATA_MAX_LENGTH - 1];
        uint8_t dcs;
        uint8_t postamble;
    };
    uint8_t raw[PN532_FRAME_DATA_MAX_LENGTH + 7];

} pn532_frame_t;

void pn532_init(void);
void pn532_power_ctl(bool on_off);
bool pn532_getFirmwareVersion(uint8_t* response, uint16_t* response_length);
bool pn532_SAMConfiguration(void);
bool pn532_inListPassiveTarget(void);
bool pn532_inDataExchange(
    uint8_t* send_data, uint8_t send_data_length, uint8_t* response, uint16_t* response_length
);
bool pn532_tgGetStatus(uint8_t* status);

#endif
