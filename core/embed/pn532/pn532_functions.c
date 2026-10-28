#include <string.h>

#include "memzero.h"
#include "pn532_defines.h"
#include "pn532_hal.h"
#include "pn532_frame.h"
#include "pn532_functions.h"

bool ReadACK(void)
{
    bool result = false;
    PN532_FRAME* frame = NULL;
    if ( pn532_frame_read(&frame) && frame != NULL )
    {
        result = frame->type == PN532_FRAME_ACK;
    }
    pn532_frame_discard(frame);
    return result;
}
bool WriteACK(void)
{
    bool result = false;
    PN532_FRAME* frame = pn532_frame_pack(PN532_FRAME_ACK, NULL, 0);
    if ( frame != NULL )
    {
        result = pn532_frame_write(frame);
    }
    pn532_frame_discard(frame);
    return result;
}
bool ExecCommand_ADV(
    bool use_extended_frame, uint8_t command, uint8_t* params, uint16_t params_length, uint8_t* response,
    uint16_t* response_length, uint32_t timeout_ms
)
{
    bool result = false;
    PN532_FRAME* frame_req = NULL;
    PN532_FRAME* frame_resp = NULL;

    // Packet data (TFI + CMD + params) is bounded by the chip's 265-byte
    // ceiling for both frame types - never by the 16-bit extended LEN field.
    uint8_t buff[PN532_FRAME_DATA_MAX];
    uint8_t* p_buff = buff;

    if ( params_length > sizeof(buff) - 2 )
        return false;
    if ( params_length > 0 && params == NULL )
        return false;

    p_buff[0] = PN532_HOSTTOPN532;
    p_buff++;

    p_buff[0] = command;
    p_buff++;

    memcpy(p_buff, params, params_length);
    p_buff += params_length;

    frame_req = pn532_frame_pack(
        use_extended_frame ? PN532_FRAME_EXTENDED_INFO : PN532_FRAME_NORMAL_INFO, buff, p_buff - buff
    );
    if ( frame_req == NULL )
    {
        goto cleanup;
    }
    if ( !pn532_frame_write(frame_req) )
    {
        goto cleanup;
    }
    pn532_frame_discard(frame_req);
    frame_req = NULL;

    if ( !pn532_hal_wait_ready(timeout_ms) )
    {
        goto cleanup;
    }

    if ( !ReadACK() )
    {
        goto cleanup;
    }

    if ( !pn532_hal_wait_ready(timeout_ms) )
    {
        goto cleanup;
    }

    if ( !pn532_frame_read(&frame_resp) )
    {
        goto cleanup;
    }
    if ( frame_resp->length < 2 )
    {
        goto cleanup;
    }
    if ( !(frame_resp->data[0] == PN532_PN532TOHOST && frame_resp->data[1] == (command + 0x01)) )
    {
        goto cleanup;
    }
    if ( frame_resp->length - 2 > *response_length )
    {
        goto cleanup;
    }
    *response_length = frame_resp->length - 2;
    memcpy(response, frame_resp->data + 2, frame_resp->length - 2);
    pn532_frame_discard(frame_resp);
    frame_resp = NULL;

    if ( !WriteACK() ) // optional, abort if anything still on going
    {
        goto cleanup;
    }

    result = true;

cleanup:
    memzero(buff, sizeof(buff));
    if ( frame_req != NULL )
        pn532_frame_discard(frame_req);
    if ( frame_resp != NULL )
        pn532_frame_discard(frame_resp);
    return result;
}

bool WriteRegister(uint16_t reg_addr, const uint8_t* const reg_val)
{
    uint8_t buff[3];
    uint16_t response_size = 0;

    buff[0] = (uint8_t)(reg_addr >> 8);
    buff[1] = (uint8_t)(reg_addr);
    buff[2] = *reg_val;
    if ( !ExecCommand(PN532_COMMAND_WRITEREGISTER, buff, 3, buff, &response_size, PN532_COMMAND_TIMEOUT) )
        return false;
    return true;
}

bool SetParameters(uint8_t flags)
{
    uint8_t buff[1];
    uint16_t response_size = 0;

    buff[0] = flags;

    if ( !ExecCommand(PN532_COMMAND_SETPARAMETERS, buff, 1, buff, &response_size, PN532_COMMAND_TIMEOUT) )
        return false;

    return true;
}

bool SAMConfiguration(PN532_SAM_MODE mode, uint8_t timeout, bool use_irq)
{
    uint8_t buff[PN532_SAM_CONFIG_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_SAM_CONFIG_BUFF_SIZE;
    p_buff = buff;
    p_buff[0] = mode;
    p_buff++;
    p_buff[0] = timeout;
    p_buff++;
    p_buff[0] = use_irq ? 0x01 : 0x00;
    p_buff++;
    if ( !ExecCommand(
             PN532_COMMAND_SAMCONFIGURATION, buff, p_buff - buff, buff, &response_size, PN532_COMMAND_TIMEOUT
         ) )
        return false;
    return true;
}

// RF communication
bool RFConfiguration(PN532_RFConfiguration_CfgItem cfg_item_type, uint8_t* data_cfg, uint16_t data_len)
{
    uint8_t buff[PN532_RFConfiguration_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_RFConfiguration_BUFF_SIZE;
    switch ( cfg_item_type )
    {
    case PN532_RFConfiguration_CfgItem_RFField:
        if ( data_len != 1 )
            return false;
        break;
    case PN532_RFConfiguration_CfgItem_VariousTimings:
        if ( data_len != 3 )
            return false;
        break;
    case PN532_RFConfiguration_CfgItem_MaxRtyCOM:
        if ( data_len != 1 )
            return false;
        break;
    case PN532_RFConfiguration_CfgItem_MaxRetries:
        if ( data_len != 3 )
            return false;
        break;
    case PN532_RFConfiguration_CfgItem_AnalogSettings_106k_typeA:
        if ( data_len != 11 )
            return false;
        break;
    case PN532_RFConfiguration_CfgItem_AnalogSettings_212k_424k:
        if ( data_len != 8 )
            return false;
        break;
    case PN532_RFConfiguration_CfgItem_AnalogSettings_typeB:
        if ( data_len != 3 )
            return false;
        break;
    case PN532_RFConfiguration_CfgItem_AnalogSettings_212k_424k_848k_iso4443dash4:
        if ( data_len != 9 )
            return false;
        break;

    default:
        return false;
        break;
    }
    p_buff = buff;
    p_buff[0] = cfg_item_type;
    p_buff++;
    memcpy(p_buff, data_cfg, data_len);
    p_buff += data_len;
    if ( !ExecCommand(
             PN532_COMMAND_RFCONFIGURATION, buff, p_buff - buff, buff, &response_size, PN532_COMMAND_TIMEOUT
         ) )
        return false;
    return true;
}

bool InListPassiveTarget(PN532_InListPassiveTarget_Params params, PN532_InListPassiveTarget_Results* results)
{
    uint8_t buff[PN532_InListPassiveTarget_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_InListPassiveTarget_BUFF_SIZE;
    if ( (params.MaxTg <= 0) || (params.MaxTg > 2) )
        return false;
    p_buff = buff;
    p_buff[0] = params.MaxTg;
    p_buff++;
    p_buff[0] = PN532_InListPassiveTarget_BrTy_106k_typeA;
    p_buff++;
    if ( !ExecCommand(
             PN532_COMMAND_INLISTPASSIVETARGET, buff, p_buff - buff, buff, &response_size,
             PN532_COMMAND_TIMEOUT
         ) )
        return false;
    if ( response_size < 1 )
        return false;

    results->NbTg = buff[0];

    uint16_t target_bytes = response_size - 1;
    if ( target_bytes > sizeof(results->TargetData) )
        target_bytes = sizeof(results->TargetData);
    memcpy(results->TargetData, &buff[1], target_bytes);
    results->TargetData_len = (uint8_t)target_bytes;

    return true;
}

bool InDataExchange_ADV(
    uint8_t Tg, uint8_t* DataOut, uint16_t DataOut_len, uint8_t* Status, uint8_t* DataIn,
    uint16_t* DataIn_len, uint32_t timeout_ms
)
{
    uint8_t buff[PN532_InDataExchange_BUFF_SIZE];
    uint8_t* p_buff = NULL;
    uint16_t response_size = PN532_InDataExchange_BUFF_SIZE;

    if ( DataOut_len > PN532_DATA_MAX_LEN )
        return false;
    if ( DataOut_len > 0 && DataOut == NULL )
        return false;
    p_buff = buff;

    p_buff[0] = Tg;
    p_buff++;

    if ( DataOut_len > 0 )
        memcpy(p_buff, DataOut, DataOut_len);
    p_buff += DataOut_len;

    // packet data = TFI + CMD + Tg + DataOut; a normal info frame LEN
    // caps that at 255, anything larger needs an extended info frame
    // (UM0701-02 sec 7.4.7 Fig 109)
    bool use_extended = (uint16_t)(3 + DataOut_len) > 0xFF;
    bool ok =
        ExecCommand_ADV(
            use_extended, PN532_COMMAND_INDATAEXCHANGE, buff, p_buff - buff, buff, &response_size, timeout_ms
        ) &&
        response_size >= 1;

    if ( ok )
    {
        *Status = buff[0];
        uint16_t data_len = response_size - 1;
        if ( data_len > *DataIn_len )
        {
            ok = false;
        }
        else
        {
            if ( data_len > 0 )
                memcpy(DataIn, &buff[1], data_len);
            *DataIn_len = data_len;
        }
    }
    memzero(buff, sizeof(buff));
    return ok;
}

bool InRelease(uint8_t Tg, uint8_t* Status)
{
    uint8_t buff[1];
    uint16_t response_size = 1;

    buff[0] = Tg;

    if ( !ExecCommand(PN532_COMMAND_INRELEASE, buff, 1, buff, &response_size, PN532_COMMAND_TIMEOUT) )
        return false;

    if ( response_size < 1 )
        return false;

    if ( Status != NULL )
        *Status = buff[0];

    return true;
}
