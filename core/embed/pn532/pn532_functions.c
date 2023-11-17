
// stdlib
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// pn532 lib
#include "pn532_defines.h"
#include "pn532_interface.h"
#include "pn532_hal.h"
#include "pn532_frame.h"
// own header
#include "pn532_functions.h"

bool ReadACK()
{
    bool result = false;
    PN532_FRAME* frame = NULL;
    if ( ReadFrame(&frame) )
    {
        result = frame->type == PN532_FRAME_ACK;
    }
    DiscardFrame(frame);
    return result;
}
bool WriteACK()
{
    bool result = false;
    PN532_FRAME* frame = PackFrame(PN532_FRAME_ACK, NULL, 0);
    if ( frame != NULL )
    {
        result = WriteFrame(frame);
    }
    DiscardFrame(frame);
    return result;
}
bool WriteNACK()
{
    bool result = false;
    PN532_FRAME* frame = PackFrame(PN532_FRAME_NACK, NULL, 0);
    if ( frame != NULL )
    {
        result = WriteFrame(frame);
    }
    DiscardFrame(frame);
    return result;
}
void WakeUp()
{
    pn532_interface->chip_sel_ctl(false);
    pn532_interface->delay_ms(2);

    // do not pull up, send cmd instead, according to FIG.49
    // pn532_interface->chip_sel_ctl(true);

    SAMConfiguration_Default();
}
bool ExecCommand_ADV(
    bool use_extended_frame, uint8_t command, uint8_t* params, uint16_t params_length, uint8_t* response,
    uint16_t* response_length, uint32_t timeout_ms
)
{
    uint8_t
        buff[use_extended_frame ? PN532_FRAME_EXTENDED_INFO_MAX_LENGTH : PN532_FRAME_NORMAL_INFO_MAX_LENGTH];
    uint8_t* p_buff = buff;

    p_buff[0] = PN532_HOSTTOPN532;
    p_buff++;

    p_buff[0] = command;
    p_buff++;

    memcpy(p_buff, params, params_length);
    p_buff += params_length;

    PN532_FRAME* frame_req = PackFrame(
        use_extended_frame ? PN532_FRAME_EXTENDED_INFO : PN532_FRAME_NORMAL_INFO, buff, p_buff - buff
    );
    if ( !WriteFrame(frame_req) )
    {
        return false;
    }
    DiscardFrame(frame_req);

    if ( !wait_ready(timeout_ms) )
    {
        return false;
    }

    if ( !ReadACK() )
    {
        return false;
    }

    if ( !wait_ready(timeout_ms) )
    {
        return false;
    }

    PN532_FRAME* frame_resp = NULL;
    if ( !ReadFrame(&frame_resp) )
    {
        return false;
    }
    if ( !(frame_resp->data[0] == PN532_PN532TOHOST && frame_resp->data[1] == (command + 0x01)) )
    {
        return false;
    }
    if ( frame_resp->length - 2 > *response_length )
    {
        return false;
    }
    *response_length = frame_resp->length - 2;
    // memcpy(response, &frame_resp->data[2], frame_resp->length - 2);
    memcpy(response, frame_resp->data + 2, frame_resp->length - 2);
    DiscardFrame(frame_req);

    if ( !WriteACK() ) // optional, abort if anything still on going
    {
        return false;
    }

    return true;
}

// Miscellaneous
bool Diagnose(PN532_DIAG diag, uint8_t* diag_params, uint8_t diag_params_len)
{
    uint8_t buff[PN532_DIAG_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_DIAG_BUFF_SIZE;

    // prepare request
    p_buff = buff;

    p_buff[0] = diag;
    p_buff++;

    memcpy(p_buff, diag_params, diag_params_len);
    p_buff += diag_params_len;

    // exec
    if ( !ExecCommand(
             PN532_COMMAND_DIAGNOSE, buff, p_buff - buff, buff, &response_size, PN532_COMMAND_TIMEOUT
         ) )
        return false;

    // parse result
    p_buff = buff;
    switch ( diag )
    {
        // case PN532_DIAG_CommunicationLineTest:
        //     break;

    case PN532_DIAG_ROMTest:
    case PN532_DIAG_RAMTest:
        return p_buff[0] == 0x00;

        // case PN532_DIAG_PollingTestToTarget:
        //     break;
        // case PN532_DIAG_EchoBackTest:
        //     break;
        // case PN532_DIAG_AttentionRequestTest:
        //     break;
        // case PN532_DIAG_SelfAntenaTest:
        //     break;

    default:
        return false;
        break;
    }
}

bool GetFirmwareVersion(PN532_FW_VER* fw_ver)
{
    uint8_t buff[PN532_FW_VER_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_FW_VER_BUFF_SIZE;

    // prepare request
    p_buff = buff;
    // command has no param

    // exec
    if ( !ExecCommand(
             PN532_COMMAND_GETFIRMWAREVERSION, buff, p_buff - buff, buff, &response_size,
             PN532_COMMAND_TIMEOUT
         ) )
        return false;

    // parse result
    p_buff = buff;
    memcpy(fw_ver, p_buff, 4);

    return true;
}

bool GetGeneralStatus();

bool ReadRegister();
bool WriteRegister();

bool ReadGPIO();
bool WriteGPIO();

bool SetSerialBaudRate();
bool SetParameters();

bool SAMConfiguration(PN532_SAM_MODE mode, uint8_t timeout, bool use_irq)
{
    uint8_t buff[PN532_SAM_CONFIG_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_SAM_CONFIG_BUFF_SIZE;

    // prepare request
    p_buff = buff;
    p_buff[0] = mode;
    p_buff++;
    p_buff[0] = timeout;
    p_buff++;
    p_buff[0] = use_irq ? 0x01 : 0x00;
    p_buff++;

    // exec
    if ( !ExecCommand(
             PN532_COMMAND_SAMCONFIGURATION, buff, p_buff - buff, buff, &response_size, PN532_COMMAND_TIMEOUT
         ) )
        return false;

    // parse result
    // nothing to do

    return true;
}

bool PowerDown();

// RF communication
bool RFConfiguration();
bool RFRegulationTest();

// Initiator
bool InJumpForDEP();
bool InJumpForPSL();
bool InListPassiveTarget(PN532_InListPassiveTarget_Params params, PN532_InListPassiveTarget_Results* results)
{
    uint8_t buff[PN532_InListPassiveTarget_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_InListPassiveTarget_BUFF_SIZE;

    // sanity check
    if ( (params.MaxTg <= 0) || (params.MaxTg > 2) ||
         ((params.BrTy == PN532_InListPassiveTarget_BrTy_106k_Jewel) && (params.MaxTg > 1)) )
        return false;

    // prepare request
    p_buff = buff;
    p_buff[0] = params.MaxTg;
    p_buff++;
    p_buff[0] = params.BrTy;
    p_buff++;

    switch ( params.BrTy )
    {
    case PN532_InListPassiveTarget_BrTy_106k_typeA:
        // InitiatorData optional
        if ( params.InitiatorData_len > 0 )
        {
            memcpy(p_buff, params.InitiatorData, params.InitiatorData_len);
            p_buff += params.InitiatorData_len;
        }
        break;

    case PN532_InListPassiveTarget_BrTy_212k:
        // InitiatorData required
        if ( params.InitiatorData_len > 0 )
        {
            memcpy(p_buff, params.InitiatorData, params.InitiatorData_len);
            p_buff += params.InitiatorData_len;
        }
        else
        {
            return false;
        }
        break;

    case PN532_InListPassiveTarget_BrTy_424k:
        // InitiatorData required
        if ( params.InitiatorData_len > 0 )
        {
            memcpy(p_buff, params.InitiatorData, params.InitiatorData_len);
            p_buff += params.InitiatorData_len;
        }
        else
        {
            return false;
        }
        break;

    case PN532_InListPassiveTarget_BrTy_106k_typeB:
        // InitiatorData required
        if ( params.InitiatorData_len > 0 )
        {
            memcpy(p_buff, params.InitiatorData, params.InitiatorData_len);
            p_buff += params.InitiatorData_len;
        }
        else
        {
            return false;
        }
        break;

    case PN532_InListPassiveTarget_BrTy_106k_Jewel:
        // InitiatorData forbidden
        if ( params.InitiatorData_len != 0 )
        {
            return false;
        }
        break;

    default:
        return false;
        break;
    }

    // exec
    if ( !ExecCommand(
             PN532_COMMAND_INLISTPASSIVETARGET, buff, p_buff - buff, buff, &response_size,
             PN532_COMMAND_TIMEOUT
         ) )
        return false;

    // parse result
    p_buff = buff;

    results->BrTy = params.BrTy;

    results->NbTg = p_buff[0];
    p_buff++;

    memcpy(results->TargetData, p_buff, PN532_InListPassiveTarget_DATA_BUFF_SIZE - (p_buff - buff));

    return true;
}
bool InATR();
bool InPSL();
bool InDataExchange(
    uint8_t Tg, uint8_t* DataOut, uint16_t DataOut_len, uint8_t* Status, uint8_t* DataIn, uint16_t* DataIn_len
)
{
    uint8_t buff[PN532_InDataExchange_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_InDataExchange_BUFF_SIZE;

    // prepare request
    p_buff = buff;

    p_buff[0] = Tg;
    p_buff++;

    memcpy(p_buff, DataOut, DataOut_len);
    p_buff += DataOut_len;

    // exec
    if ( !ExecCommand(
             PN532_COMMAND_INDATAEXCHANGE, buff, p_buff - buff, buff, &response_size, PN532_COMMAND_TIMEOUT
         ) )
        return false;

    // parse result
    p_buff = buff;

    *Status = p_buff[0];
    p_buff++;

    memset(DataIn, 0x00, *DataIn_len);
    memcpy(DataIn, p_buff, response_size);
    *DataIn_len = response_size;

    return true;
}
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