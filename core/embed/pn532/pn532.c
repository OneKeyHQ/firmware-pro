#include <string.h>

#include "pn532.h"
#include "systick.h"

static const uint8_t pn532_ack_frame[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
static pn532_controller_t pn532_controller = {0};

void pn532_init(void)
{
    pn532_controller.spi_controller = get_spi_controller();
    pn532_controller.stub_controller = get_stub_controller();
    pn532_controller.delay_ms = dwt_delay_ms;
    pn532_controller.stub_controller->init();
    pn532_controller.spi_controller->spi_init();

    pn532_controller.stub_controller->chip_reset_ctl(false);
    // pn532_controller.delay_ms(5);
    // pn532_controller.stub_controller->chip_reset_ctl(true);
    // pn532_controller.delay_ms(20);
}

void pn532_power_ctl(bool on_off)
{
    if ( on_off )
    {
        pn532_controller.spi_controller->spi_init();
        pn532_controller.stub_controller->chip_reset_ctl(false);
        pn532_controller.delay_ms(1);
        pn532_controller.stub_controller->chip_reset_ctl(true);
        pn532_controller.delay_ms(50);
    }
    else
    {
        pn532_controller.stub_controller->chip_reset_ctl(false);
        pn532_controller.spi_controller->spi_deinit();
        pn532_controller.delay_ms(5);
    }
}

static bool pn532_write_frame(pn532_controller_t* pn532, uint8_t cmd, uint8_t* params, uint8_t params_length)
{
    pn532_frame_t frame = {0};
    uint8_t spi_cmd = PN532_SPI_DATAWRITE;

    if ( params_length > PN532_FRAME_DATA_MAX_LENGTH - 1 )
    {
        return false;
    }

    frame.preamble = PN532_PREAMBLE;
    frame.start_code1 = PN532_STARTCODE1;
    frame.start_code2 = PN532_STARTCODE2;
    frame.len = params_length + 2;
    frame.lcs = ~frame.len + 1;
    frame.tfi = PN532_HOSTTOPN532;
    frame.data[0] = cmd;
    memcpy(frame.data + 1, params, params_length);
    frame.dcs = 0;
    for ( uint16_t i = 0; i < frame.len; i++ )
    {
        frame.dcs += frame.raw[5 + i];
    }
    frame.dcs = ~frame.dcs + 1;
    frame.data[params_length + 1] = frame.dcs;
    frame.data[params_length + 2] = PN532_POSTAMBLE;
    pn532->spi_controller->chip_select(true);
    pn532->delay_ms(1);
    pn532->spi_controller->write(&spi_cmd, 1);

    for ( uint16_t i = 0; i < frame.len + 7; i++ )
    {
        pn532->spi_controller->write(frame.raw + i, 1);
    }
    // pn532->spi_controller->write(frame.raw, frame.len + 7);
    pn532->spi_controller->chip_select(false);
    pn532->delay_ms(1);

    return true;
}

static bool pn532_get_status(pn532_controller_t* pn532)
{
    uint8_t status = 0;
    uint8_t cmd = PN532_SPI_STATREAD;
    pn532->spi_controller->chip_select(true);
    pn532->delay_ms(1);
    pn532->spi_controller->write(&cmd, 1);
    pn532->spi_controller->read(&status, 1);
    pn532->spi_controller->chip_select(false);
    pn532->delay_ms(1);
    return status == PN532_SPI_READY;
}

static bool pn532_wait_ready(pn532_controller_t* pn532, int timeout_ms)
{
    while ( !pn532_get_status(pn532) )
    {
        if ( timeout_ms <= 0 )
        {
            return false;
        }
        pn532->delay_ms(10);
        timeout_ms -= 10;
    }
    return true;
}

static bool pn532_read_ack(pn532_controller_t* pn532)
{
    uint8_t cmd = PN532_SPI_DATAREAD;
    uint8_t ack[6] = {0};
    pn532->spi_controller->chip_select(true);
    pn532->spi_controller->write(&cmd, 1);
    pn532->spi_controller->read(ack, sizeof(ack));
    pn532->spi_controller->chip_select(false);
    return memcmp(ack, pn532_ack_frame, sizeof(ack)) == 0;
}

static bool pn532_read_frame(pn532_controller_t* pn532, pn532_frame_t* frame)
{
    uint8_t cmd = PN532_SPI_DATAREAD;
    uint8_t index = 0;
    bool ret = false;
    pn532->spi_controller->chip_select(true);
    pn532->spi_controller->write(&cmd, 1);
    pn532->spi_controller->read(frame->raw, 3);
    if ( frame->preamble != PN532_PREAMBLE || frame->start_code1 != PN532_STARTCODE1 ||
         frame->start_code2 != PN532_STARTCODE2 )
    {
        goto exit;
    }
    index = 3;

    pn532->spi_controller->read(frame->raw + index, 2);
    index += 2;

    if ( frame->len > PN532_FRAME_DATA_MAX_LENGTH )
    {
        goto exit;
    }

    if ( frame->len != ((~frame->lcs + 1) & 0xff) )
    {
        goto exit;
    }

    for ( uint16_t i = 0; i < frame->len + 2; i++ )
    {
        pn532->spi_controller->read(frame->raw + index, 1);
        index++;
    }

    // pn532->spi_controller->read(frame->raw + index, frame->len + 2);

    if ( frame->tfi != PN532_PN532TOHOST )
    {
        goto exit;
    }

    uint8_t dcs = 0;
    for ( uint16_t i = 0; i < frame->len + 2; i++ )
    {
        dcs += frame->raw[5 + i];
    }

    if ( dcs != 0 )
    {
        goto exit;
    }

    ret = true;

exit:
    pn532->spi_controller->chip_select(false);
    return ret;
}

static bool
pn532_write_cmd_check_ack(pn532_controller_t* pn532, uint8_t cmd, uint8_t* params, uint8_t params_length)
{
    pn532_write_frame(pn532, cmd, params, params_length);
    if ( !pn532_wait_ready(pn532, PN532_TIMEOUT_MS_NORMAL) )
    {
        return false;
    }
    return pn532_read_ack(pn532);
}

static bool
pn532_read_response(pn532_controller_t* pn532, uint8_t* resonse, uint16_t* resonse_len, int timeout_ms)
{
    pn532_frame_t frame = {0};
    if ( !pn532_wait_ready(pn532, timeout_ms) )
    {
        return false;
    }

    if ( !pn532_read_frame(pn532, &frame) )
    {
        return false;
    }
    if ( resonse != NULL )
    {
        uint8_t len = *resonse_len > frame.len - 2 ? frame.len - 2 : *resonse_len;
        memcpy(resonse, frame.data + 1, len);
        *resonse_len = len;
    }

    return true;
}

bool pn532_transceive(
    uint8_t cmd, uint8_t* paras, uint8_t paras_length, uint8_t* response, uint16_t* response_length,
    int timeout_ms
)
{
    if ( !pn532_write_cmd_check_ack(&pn532_controller, cmd, paras, paras_length) )
    {
        return false;
    }
    return pn532_read_response(&pn532_controller, response, response_length, timeout_ms);
}

bool pn532_getFirmwareVersion(uint8_t* response, uint16_t* response_length)
{
    return pn532_transceive(
        PN532_COMMAND_GETFIRMWAREVERSION, NULL, 0, response, response_length, PN532_TIMEOUT_MS_NORMAL
    );
}

bool pn532_SAMConfiguration(void)
{
    uint8_t params[] = {0x01, 0x14, 0x01};
    return pn532_transceive(
        PN532_COMMAND_SAMCONFIGURATION, params, sizeof(params), NULL, NULL, PN532_TIMEOUT_MS_NORMAL
    );
}

bool pn532_inListPassiveTarget(void)
{
    uint8_t params[] = {0x01, PN532_InListPassiveTarget_BrTy_106k_typeA};
    uint8_t response[PN532_FRAME_DATA_MAX_LENGTH] = {0};
    uint16_t response_length = sizeof(response);
    if ( !pn532_transceive(
             PN532_COMMAND_INLISTPASSIVETARGET, params, sizeof(params), response, &response_length,
             PN532_TIMEOUT_MS_NORMAL_PASSSIVETARGET
         ) )
    {
        return false;
    }
    if ( response[0] != 0x01 )
    {
        return false;
    }
    return true;
}

bool pn532_inDataExchange(
    uint8_t* send_data, uint8_t send_data_length, uint8_t* response, uint16_t* response_length
)
{
    uint8_t buffer[PN532_FRAME_DATA_MAX_LENGTH] = {0};
    uint16_t recv_len = sizeof(buffer);
    if ( send_data_length > PN532_FRAME_DATA_MAX_LENGTH - 1 )
    {
        return false;
    }
    buffer[0] = 0x01;
    memcpy(buffer + 1, send_data, send_data_length);
    if ( !pn532_transceive(
             PN532_COMMAND_INDATAEXCHANGE, buffer, send_data_length + 1, buffer, &recv_len,
             PN532_TIMEOUT_MS_DATA_EXCHANGE
         ) )
    {
        return false;
    }
    if ( buffer[0] != 0x00 )
    {
        return false;
    }
    memmove(response, buffer + 1, recv_len - 1);
    *response_length = recv_len - 1;
    return true;
}

bool pn532_tgGetStatus(uint8_t* status)
{
    uint8_t response[PN532_FRAME_DATA_MAX_LENGTH] = {0};
    uint16_t response_length = sizeof(response);
    if ( !pn532_transceive(
             PN532_COMMAND_TGGETTARGETSTATUS, NULL, 0, response, &response_length, PN532_TIMEOUT_MS_NORMAL
         ) )
    {
        return false;
    }
    *status = response[0];
    return true;
}
