#ifndef _PN532_FUNCTIONS_H_
#define _PN532_FUNCTIONS_H_

#include <stdint.h>
#include <stdbool.h>

#include "pn532_defines.h"

bool ReadACK();
bool WriteACK();
bool WriteNACK();
void WakeUp();
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

// Miscellaneous
bool Diagnose(PN532_DIAG diag, uint8_t* params, uint8_t param_cout);

bool GetFirmwareVersion(PN532_FW_VER* fw_ver);

bool GetGeneralStatus();

bool ReadRegister();
bool WriteRegister();

bool ReadGPIO();
bool WriteGPIO();

bool SetSerialBaudRate();
bool SetParameters();

bool SAMConfiguration(PN532_SAM_MODE mode, uint8_t timeout, bool use_irq);
#define SAMConfiguration_Default() SAMConfiguration(PN532_SAM_Normal, 0, true)

bool PowerDown();

// RF communication
bool RFConfiguration();
bool RFRegulationTest();

// Initiator
bool InJumpForDEP();
bool InJumpForPSL();
bool InListPassiveTarget(PN532_InListPassiveTarget_Params params, PN532_InListPassiveTarget_Results* results);
bool InATR();
bool InPSL();
bool InDataExchange(
    uint8_t Tg, uint8_t* DataOut, uint16_t DataOut_len, uint8_t* Status, uint8_t* DataIn, uint16_t* DataIn_len
);
bool InCommunicateThru();
bool InDeselect();
bool InRelease();
bool InSelect();
bool InAutoPoll();

// Target
bool TgInitAsTarget();
bool TgSetGeneralBytes();
bool TgGetData();
bool TgSetData();
bool TgSetMetaData();
bool TgGetInitiatorCommand();
bool TgResponseToInitiator();
bool TgGetTargetStatus();

#endif // _PN532_FUNCTIONS_H_