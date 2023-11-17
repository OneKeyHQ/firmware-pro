#ifndef _PN532_INTERNAL_
#define _PN532_INTERNAL_
// stdlib
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// pn532 lib
#include "pn532_defines.h"
#include "pn532_interface.h"
#include "pn532_hal.h"
// own header
#include "pn532_frame.h"

#ifndef PN532_SUPPORT_EXTENDED_INFO_FRAME
  #pragma message "Note: PN532 Extended Information Frame Disabled"
#else
  #pragma message "Note: PN532 Extended Information Frame Enabled"
#endif

// Frame Container
PN532_FRAME* PackFrame(PN532_FRAME_TYPE type, uint8_t* buff, uint8_t len)
{
    PN532_FRAME* frame = (PN532_FRAME*)malloc(sizeof(PN532_FRAME) + sizeof(uint8_t) * len);
    if ( frame != NULL )
    {
        frame->type = type;
        frame->length = len;
        if ( buff != NULL )
        {
            memcpy(frame->data, buff, len);
            // memcpy(&frame->data[0], buff, len); // or this?
        }
    }
    return frame;
}

void DiscardFrame(PN532_FRAME* frame)
{
    if ( frame != NULL )
    {
        free(frame);
        frame = NULL;
    }
}

// Frame RW

bool WriteFrame(PN532_FRAME* frame)
{
#ifdef PN532_DEBUG
    pn532_interface->log("%s\n", __func__);
#endif

    uint8_t frame_buffer[PN532_FRAME_MAX_LENGTH];
    uint16_t frame_raw_size = 0;
    // uint16_t frame_data_len = 0;
    // uint8_t frame_data_checksum = 0xff;

    memset(frame_buffer, PN532_FRAME_MAX_LENGTH, 0x00);

    // check type and data length
    if ( frame->type == PN532_FRAME_NORMAL_INFO || frame->type == PN532_FRAME_EXTENDED_INFO )
    {
        // check direction (TFI)
        if ( frame->data[0] != PN532_HOSTTOPN532 )
        {
            pn532_interface->log("Frame direction mark invalid!\n");
            return false;
        }

        // check length sanity
        if ( (frame->type == PN532_FRAME_NORMAL_INFO && frame->length > 0x00ff) ||
             (frame->type == PN532_FRAME_EXTENDED_INFO && frame->length > 0xffff) )
        {
            pn532_interface->log("Frame data length overflow for type!\n");
            return false;
        }
    }
    else if ( frame->type == PN532_FRAME_ACK || frame->type == PN532_FRAME_NACK )
    {
        // check length sanity
        if ( frame->length != 0 )
        {
            pn532_interface->log("Frame data length overflow for type!\n");
            return false;
        }
    }
    else if ( frame->type == PN532_FRAME_ERROR )
    {
        // send error frame to PN532 do not make sense
        pn532_interface->log("Frame type invalid!\n");
        return false;
    }
    else
    {
        pn532_interface->log("Frame type invalid!\n");
        return false;
    }

    // make raw frame

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
            // length
            frame_buffer[frame_raw_size] = 0xff & frame->length;
            frame_raw_size += sizeof(uint8_t);
            // length checksum
            frame_buffer[frame_raw_size] = 0xff & (~frame->length + 1);
            frame_raw_size += sizeof(uint8_t);
        }
        if ( frame->type == PN532_FRAME_EXTENDED_INFO )
        {
            // length padding
            frame_buffer[frame_raw_size] = 0xff;
            frame_buffer[frame_raw_size + 1] = 0xff;
            frame_raw_size += sizeof(uint16_t);
            // length
            frame_buffer[frame_raw_size] = (frame->length >> 8);
            frame_buffer[frame_raw_size + 1] = (frame->length & 0x00ff);
            frame_raw_size += sizeof(uint16_t);
            // length checksum
            frame_buffer[frame_raw_size] = 0xff & (~((frame->length >> 8) + (frame->length & 0x00ff)) + 1);
            frame_raw_size += sizeof(uint8_t);
        }

        // data
        memcpy(&frame_buffer[frame_raw_size], frame->data, frame->length);
        frame_raw_size += frame->length;

        // data checksum
        frame_buffer[frame_raw_size] = 0;
        for ( uint8_t i = 0; i < frame->length; i++ )
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

    // hal read (until timeout)
    if ( !write_data(frame_buffer, frame_raw_size) )
    {
        pn532_interface->log("Write data failed!\n");
        return false;
    }

    return true;
}

bool ReadFrame(PN532_FRAME** frame)
{
#ifdef PN532_DEBUG
    pn532_interface->log("%s\n", __func__);
#endif

    uint8_t frame_buffer[PN532_FRAME_MAX_LENGTH];
    uint8_t* p_frame = frame_buffer;
    uint16_t frame_data_len = 0;
    uint8_t frame_data_checksum = 0xff;
    PN532_FRAME_TYPE frame_type;

    memset(frame_buffer, PN532_FRAME_MAX_LENGTH, 0x00);

    // hal read (until timeout)
    if ( !read_data(frame_buffer, sizeof(frame_buffer)) )
    {
        pn532_interface->log("Read data failed!\n");
        return false;
    }

    // find frame header
    while ( true )
    {
        if ( p_frame >= frame_buffer + sizeof(frame_buffer) )
        {
            pn532_interface->log("Frame header not found!\n");
            return false;
        }

        if ( p_frame[0] == 0x00 && p_frame[1] == 0xff )
        {
            p_frame += 2; // skip 0x00ff
            break;
        }
        else
        {
            p_frame++;
        }
    }

    // check type and data length
    if ( ((p_frame[0] + p_frame[1]) & 0xff) == 0 )
    {
        frame_type = PN532_FRAME_NORMAL_INFO;
        frame_data_len = p_frame[0];
        p_frame += 2; // skip length and length checksum
    }
    else if ( (p_frame[0] == 0xff) && (p_frame[1] == 0xff) && (((p_frame[2] + p_frame[3] + p_frame[4]) & 0xffff) == 0) )
    {
        frame_type = PN532_FRAME_EXTENDED_INFO;
        frame_data_len = (p_frame[2] << 8) + p_frame[3];
        p_frame += 5; // skip length and length checksum
    }
    else if ( (p_frame[0] == 0x00) && (p_frame[1] == 0xff) )
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
        frame_data_len = p_frame[0]; // always be 1
        p_frame += 2;                // skip length and length checksum
    }
    else
    {
        pn532_interface->log("Frame corrupted or invalid!\n");
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
            pn532_interface->log("Frame direction mark invalid!\n");
            return false;
        }
        // verify data with checksum
        frame_data_checksum = 0;
        for ( uint8_t i = 0; i < frame_data_len; i++ )
        {
            frame_data_checksum += p_frame[i];
        }
        frame_data_checksum += p_frame[frame_data_len]; // the actual checksum from raw frame
        if ( frame_data_checksum != 0 )
        {
            pn532_interface->log("Frame data or checksum corrupted\n");
            return false;
        }
        // pack frame
        *frame = PackFrame(frame_type, p_frame, frame_data_len);
        return true;
        // break;

    case PN532_FRAME_ACK:
    case PN532_FRAME_NACK:
        *frame = PackFrame(frame_type, NULL, frame_data_len);
        return true;
        // break;

    case PN532_FRAME_ERROR:
        pn532_interface->log("Error frame recived\n");
        // verify data with checksum
        if ( ((p_frame[0] + p_frame[1]) & 0xff) != 0 )
        {
            pn532_interface->log("Frame data or checksum corrupted\n");
            return false;
        }
        pn532_interface->log("Error code: %00X\n", p_frame[0]);
        *frame = PackFrame(frame_type, p_frame, frame_data_len);
        return true;
        // break;

    default:
        break;
    }

    // #ifdef PN532_DEBUG
    //     pn532_interface->log("Frame length: %u\n", len);

    //     size_t byte_str_len = (len) * 3 + 1;
    //     char str[byte_str_len];
    //     size_t processed = 0;
    //     memzero(str, byte_str_len);
    //     if ( buffer_to_hex_string(buff, len, str, byte_str_len, &processed) )
    //         pn532_interface->log("Frame data: %s\n", str);
    //     else
    //         pn532_interface->log("Frame data: convert failed\n");
    // #endif

    // should never reach here
    return false;
}

#endif // _PN532_INTERNAL_