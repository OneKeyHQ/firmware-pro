#include <string.h>

#include "memzero.h"
#include "pn532_defines.h"
#include "pn532_hal.h"
#include "pn532_frame.h"

// Two-frame static pool: sized for simultaneous request + response in
// ExecCommand_ADV. Not safe for nested or concurrent callers.
static PN532_FRAME static_frame_tx;
static PN532_FRAME static_frame_rx;
static bool frame_tx_in_use = false;
static bool frame_rx_in_use = false;

PN532_FRAME* pn532_frame_pack(PN532_FRAME_TYPE type, uint8_t* buff, uint16_t len)
{
    if ( len > PN532_FRAME_DATA_MAX )
        return NULL;

    // Allocate from static pool (TX for pn532_frame_write callers, RX for pn532_frame_read callers)
    PN532_FRAME* frame;
    if ( !frame_tx_in_use )
    {
        frame = &static_frame_tx;
        frame_tx_in_use = true;
    }
    else if ( !frame_rx_in_use )
    {
        frame = &static_frame_rx;
        frame_rx_in_use = true;
    }
    else
    {
        return NULL; // Both frames in use -- caller must pn532_frame_discard first
    }

    frame->type = type;
    frame->length = len;
    if ( buff != NULL )
    {
        memcpy(frame->data, buff, frame->length);
    }
    return frame;
}

void pn532_frame_discard(PN532_FRAME* frame)
{
    // Frames can carry card APDUs in clear, so the static pool must not keep them.
    if ( frame == &static_frame_tx )
    {
        memzero(frame, sizeof(*frame));
        frame_tx_in_use = false;
    }
    else if ( frame == &static_frame_rx )
    {
        memzero(frame, sizeof(*frame));
        frame_rx_in_use = false;
    }
}

// Frame RW

bool pn532_frame_write(PN532_FRAME* frame)
{

    uint8_t frame_buffer[PN532_FRAME_RAW_MAX];
    uint16_t frame_raw_size = 0;

    memset(frame_buffer, 0x00, sizeof(frame_buffer));

    // check type and data length
    if ( frame->type == PN532_FRAME_NORMAL_INFO || frame->type == PN532_FRAME_EXTENDED_INFO )
    {
        // check direction (TFI)
        if ( frame->data[0] != PN532_HOSTTOPN532 )
        {
            return false;
        }
        if ( frame->type == PN532_FRAME_NORMAL_INFO && frame->length > 0x00ff )
        {
            return false;
        }
    }
    else if ( frame->type == PN532_FRAME_ACK || frame->type == PN532_FRAME_NACK )
    {
        if ( frame->length != 0 )
        {
            return false;
        }
    }
    else if ( frame->type == PN532_FRAME_ERROR )
    {
        // send error frame to PN532 do not make sense
        return false;
    }
    else
    {
        return false;
    }
    frame_buffer[frame_raw_size] = PN532_PREAMBLE;
    frame_raw_size += sizeof(uint8_t);

    frame_buffer[frame_raw_size] = PN532_STARTCODE1;
    frame_raw_size += sizeof(uint8_t);
    frame_buffer[frame_raw_size] = PN532_STARTCODE2;
    frame_raw_size += sizeof(uint8_t);

    if ( frame->type == PN532_FRAME_NORMAL_INFO || frame->type == PN532_FRAME_EXTENDED_INFO )
    {
        // TFI treat as part of data for all following logics
        if ( frame->type == PN532_FRAME_NORMAL_INFO )
        {
            frame_buffer[frame_raw_size] = 0xff & frame->length;
            frame_raw_size += sizeof(uint8_t);
            frame_buffer[frame_raw_size] = 0xff & (~frame->length + 1);
            frame_raw_size += sizeof(uint8_t);
        }
        if ( frame->type == PN532_FRAME_EXTENDED_INFO )
        {
            // length padding
            frame_buffer[frame_raw_size] = 0xff;
            frame_buffer[frame_raw_size + 1] = 0xff;
            frame_raw_size += sizeof(uint16_t);
            frame_buffer[frame_raw_size] = (frame->length >> 8);
            frame_buffer[frame_raw_size + 1] = (frame->length & 0x00ff);
            frame_raw_size += sizeof(uint16_t);
            frame_buffer[frame_raw_size] = 0xff & (~((frame->length >> 8) + (frame->length & 0x00ff)) + 1);
            frame_raw_size += sizeof(uint8_t);
        }
        memcpy(&frame_buffer[frame_raw_size], frame->data, frame->length);
        frame_raw_size += frame->length;

        // data checksum. i must be uint16_t: frame->length reaches 265 for an
        // extended info frame, and a uint8_t counter would wrap at 255 and
        // loop forever (extended frames are produced by any APDU >= 253 bytes,
        // e.g. the 256-byte SCP11 mutual-auth command).
        frame_buffer[frame_raw_size] = 0;
        for ( uint16_t i = 0; i < frame->length; i++ )
        {
            frame_buffer[frame_raw_size] += frame->data[i];
        }
        frame_buffer[frame_raw_size] = ~frame_buffer[frame_raw_size] + 1;
        frame_raw_size += sizeof(uint8_t);
    }
    else if ( frame->type == PN532_FRAME_ACK )
    {
        frame_buffer[frame_raw_size] = 0x00;
        frame_buffer[frame_raw_size + 1] = 0xff;
        frame_raw_size += sizeof(uint16_t);
    }
    else if ( frame->type == PN532_FRAME_NACK )
    {
        frame_buffer[frame_raw_size] = 0xff;
        frame_buffer[frame_raw_size + 1] = 0x00;
        frame_raw_size += sizeof(uint16_t);
    }

    frame_buffer[frame_raw_size] = PN532_POSTAMBLE;
    frame_raw_size += sizeof(uint8_t);

    bool written = pn532_hal_write_data(frame_buffer, frame_raw_size);
    memzero(frame_buffer, sizeof(frame_buffer));
    return written;
}

static bool pn532_frame_parse(uint8_t* frame_buffer, uint16_t frame_buffer_len, PN532_FRAME** frame)
{
    uint8_t* p_frame = frame_buffer;
    uint16_t frame_data_len = 0;
    uint8_t frame_data_checksum = 0xff;
    PN532_FRAME_TYPE frame_type;

    uint8_t* frame_end = frame_buffer + frame_buffer_len;

    // find frame header
    while ( (p_frame + 1) < frame_end )
    {
        if ( p_frame[0] == 0x00 && p_frame[1] == 0xff )
        {
            p_frame += 2; // skip 0x00ff
            break;
        }

        p_frame++;
    }

    if ( (p_frame + 1) >= frame_end )
    {
        return false;
    }

    // check type and data length
    //
    // NOTE: order matters. ACK/NACK/ERROR frame headers satisfy the
    // length+LCS sum==0 check too (01 FF, 00 FF, FF 00), so test their
    // exact patterns first before falling through to NORMAL_INFO.
    if ( (p_frame[0] == 0x00) && (p_frame[1] == 0xff) )
    {
        frame_type = PN532_FRAME_ACK;
        frame_data_len = 0;
    }
    else if ( (p_frame[0] == 0xff) && (p_frame[1] == 0x00) )
    {
        frame_type = PN532_FRAME_NACK;
        frame_data_len = 0;
    }
    else if ( (p_frame[0] == 0x01) && (p_frame[1] == 0xff) )
    {
        frame_type = PN532_FRAME_ERROR;
        frame_data_len = 1;
        p_frame += 2; // skip length and length checksum
    }
    else if ( ((p_frame[0] + p_frame[1]) & 0xff) == 0 )
    {
        frame_type = PN532_FRAME_NORMAL_INFO;
        frame_data_len = p_frame[0];
        p_frame += 2; // skip length and length checksum
    }
    else if ( (p_frame + 4) < frame_end && (p_frame[0] == 0xff) && (p_frame[1] == 0xff) && (((p_frame[2] + p_frame[3] + p_frame[4]) & 0xff) == 0) )
    {
        frame_type = PN532_FRAME_EXTENDED_INFO;
        frame_data_len = (p_frame[2] << 8) + p_frame[3];
        p_frame += 5; // skip length and length checksum
    }
    else
    {
        return false;
    }

    // need at least frame data + checksum bytes remaining
    if ( frame_data_len >= (uint16_t)(frame_end - p_frame) )
    {
        return false;
    }

    // process by type
    switch ( frame_type )
    {
    case PN532_FRAME_NORMAL_INFO:
    case PN532_FRAME_EXTENDED_INFO:
        // check direction (TFI)
        if ( p_frame[0] != PN532_PN532TOHOST )
        {
            return false;
        }
        // verify data with checksum
        frame_data_checksum = 0;
        for ( uint16_t i = 0; i < frame_data_len; i++ )
        {
            frame_data_checksum += p_frame[i];
        }
        frame_data_checksum += p_frame[frame_data_len]; // the actual checksum from raw frame
        if ( frame_data_checksum != 0 )
        {
            return false;
        }
        // pack frame
        *frame = pn532_frame_pack(frame_type, p_frame, frame_data_len);
        if ( *frame == NULL )
        {
            return false;
        }
        return true;
        // break;

    case PN532_FRAME_ACK:
    case PN532_FRAME_NACK:
        *frame = pn532_frame_pack(frame_type, NULL, frame_data_len);
        if ( *frame == NULL )
        {
            return false;
        }
        return true;
        // break;

    case PN532_FRAME_ERROR:
        // verify data with checksum
        if ( ((p_frame[0] + p_frame[1]) & 0xff) != 0 )
        {
            return false;
        }
        *frame = pn532_frame_pack(frame_type, p_frame, frame_data_len);
        if ( *frame == NULL )
        {
            return false;
        }
        return true;
        // break;

    default:
        break;
    }

    // should never reach here
    return false;
}

bool pn532_frame_read(PN532_FRAME** frame)
{
    uint8_t frame_buffer[PN532_FRAME_RAW_MAX] = {0};
    bool ok = pn532_hal_read_data(frame_buffer, sizeof(frame_buffer)) &&
              pn532_frame_parse(frame_buffer, sizeof(frame_buffer), frame);
    memzero(frame_buffer, sizeof(frame_buffer));
    return ok;
}
