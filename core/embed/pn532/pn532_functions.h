#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "pn532_defines.h"

bool ReadACK(void);
bool WriteACK(void);
bool ExecCommand_ADV(
    bool use_extended_frame, uint8_t command, uint8_t* params, uint16_t params_length, uint8_t* response,
    uint16_t* response_length, uint32_t timeout_ms
);
#ifndef PN532_SUPPORT_EXTENDED_INFO_FRAME
  #define ExecCommand(_command, _params, _params_length, _response, _response_length, _timeout_ms) \
    ExecCommand_ADV(false, _command, _params, _params_length, _response, _response_length, _timeout_ms)
#else
  #define ExecCommand(_command, _params, _params_length, _response, _response_length, _timeout_ms) \
    ExecCommand_ADV(true, _command, _params, _params_length, _response, _response_length, _timeout_ms)
#endif

bool WriteRegister(uint16_t reg_addr, const uint8_t* const reg_val);

bool SetParameters(uint8_t flags);

bool SAMConfiguration(PN532_SAM_MODE mode, uint8_t timeout, bool use_irq);

bool RFConfiguration(PN532_RFConfiguration_CfgItem cfg_item_type, uint8_t* data_cfg, uint16_t data_len);

bool InListPassiveTarget(PN532_InListPassiveTarget_Params params, PN532_InListPassiveTarget_Results* results);
// Caller-set command timeout (slow ISO 14443-4 cards extend via S(WTX) and
// the chip signals completion only when the exchange really ends), plus
// automatic extended-info-frame selection for packet data over 255 bytes.
bool InDataExchange_ADV(
    uint8_t Tg, uint8_t* DataOut, uint16_t DataOut_len, uint8_t* Status, uint8_t* DataIn,
    uint16_t* DataIn_len, uint32_t timeout_ms
);
bool InRelease(uint8_t Tg, uint8_t* Status);
