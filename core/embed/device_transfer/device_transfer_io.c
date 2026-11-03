#include "device_transfer_io.h"

#include <string.h>

#include "common.h"
#include "memzero.h"
#include "pn532_frame.h"
#include "pn532_functions.h"
#include "pn532_hal.h"

static struct
{
    uint8_t packet[PN532_FRAME_DATA_MAX];
    uint16_t length;
    uint32_t started;
    uint32_t timeout;
    uint8_t attempts;
    bool pending;
    bool acked;
} io;

static bool write_command(void)
{
    PN532_FRAME* frame = pn532_frame_pack(
        io.length > 255 ? PN532_FRAME_EXTENDED_INFO : PN532_FRAME_NORMAL_INFO, io.packet, io.length
    );
    if ( frame == NULL )
        return false;
    bool ok = pn532_frame_write(frame);
    pn532_frame_discard(frame);
    io.started = hal_ticks_ms();
    io.attempts++;
    return ok;
}

bool device_transfer_io_start(uint8_t command, const uint8_t* data, uint16_t length, uint32_t timeout_ms)
{
    if ( io.pending || length > sizeof(io.packet) - 2 || timeout_ms == 0 || (length && data == NULL) )
        return false;
    memzero(&io, sizeof(io));
    io.packet[0] = PN532_HOSTTOPN532;
    io.packet[1] = command;
    if ( length )
        memcpy(io.packet + 2, data, length);
    io.length = length + 2;
    io.timeout = timeout_ms;
    io.pending = write_command();
    return io.pending;
}

device_transfer_io_result_t device_transfer_io_poll(uint8_t* data, uint16_t* length)
{
    if ( !io.pending || data == NULL || length == NULL )
        return DEVICE_TRANSFER_IO_ERROR;
    // The UI scheduler can resume after the deadline even though the chip
    // answered in time. Consume a ready ACK/response before considering retry.
    if ( !pn532_hal_is_ready() )
    {
        uint32_t elapsed = hal_ticks_ms() - io.started;
        uint32_t budget = io.acked ? io.timeout : DEVICE_TRANSFER_ACK_TIMEOUT_MS;
        if ( elapsed < budget )
            return DEVICE_TRANSFER_IO_WAIT;
        // Retry an unacknowledged SPI write only; an acknowledged RF command is
        // never retransmitted on a UI tick.
        if ( !io.acked && io.attempts < DEVICE_TRANSFER_ACK_ATTEMPTS )
        {
            if ( write_command() )
                return DEVICE_TRANSFER_IO_WAIT;
            io.pending = false;
            return DEVICE_TRANSFER_IO_ERROR;
        }
        io.pending = false;
        return io.acked ? DEVICE_TRANSFER_IO_TIMEOUT : DEVICE_TRANSFER_IO_ERROR;
    }
    if ( !io.acked )
    {
        if ( !ReadACK() )
        {
            io.pending = false;
            return DEVICE_TRANSFER_IO_ERROR;
        }
        io.acked = true;
        io.started = hal_ticks_ms();
        return DEVICE_TRANSFER_IO_WAIT;
    }

    io.pending = false;
    PN532_FRAME* frame = NULL;
    bool ok = pn532_frame_read(&frame) && frame != NULL &&
              (frame->type == PN532_FRAME_NORMAL_INFO || frame->type == PN532_FRAME_EXTENDED_INFO) &&
              frame->length >= 2 && frame->data[0] == PN532_PN532TOHOST &&
              frame->data[1] == io.packet[1] + 1 && frame->length - 2 <= *length;
    if ( ok )
    {
        *length = frame->length - 2;
        memcpy(data, frame->data + 2, *length);
    }
    pn532_frame_discard(frame);
    if ( !ok || !WriteACK() )
        return DEVICE_TRANSFER_IO_ERROR;
    return DEVICE_TRANSFER_IO_DONE;
}

void device_transfer_io_close(void)
{
    (void)WriteACK();
    memzero(&io, sizeof(io));
}
