#include "secure_p2p_defines.h"
#include "secure_p2p_frame.h"

// ########################################################################
// public functions

static uint32_t secure_p2p_crc32(const uint8_t* data_p, size_t len)
{
    // software CRC32 (IEEE 802.3, zlib-compatible) via the shim
    return secure_channel_port_crc32(data_p, len);
}

secbool_t secure_p2p_frame_seal(uint8_t* buf_p, uint32_t* len_p, uint32_t capacity)
{
    if ( buf_p == NULL || len_p == NULL )
    {
        return secfalse;
    }
    if ( *len_p > capacity || capacity - *len_p < SECURE_P2P_CRC32_LEN )
    {
        return secfalse;
    }

    uint32_t crc = secure_p2p_crc32(buf_p, *len_p);
    buf_p[*len_p + 0] = (uint8_t)(crc >> 24);
    buf_p[*len_p + 1] = (uint8_t)(crc >> 16);
    buf_p[*len_p + 2] = (uint8_t)(crc >> 8);
    buf_p[*len_p + 3] = (uint8_t)(crc >> 0);
    *len_p += SECURE_P2P_CRC32_LEN;
    return sectrue;
}

secbool_t secure_p2p_frame_verify(const uint8_t* buf_p, uint32_t* len_p)
{
    if ( buf_p == NULL || len_p == NULL )
    {
        return secfalse;
    }
    if ( *len_p < SECURE_P2P_CRC32_LEN + 1 )
    {
        return secfalse;
    }

    uint32_t body_len = *len_p - SECURE_P2P_CRC32_LEN;
    uint32_t expected = ((uint32_t)buf_p[body_len + 0] << 24) //
                      | ((uint32_t)buf_p[body_len + 1] << 16) //
                      | ((uint32_t)buf_p[body_len + 2] << 8)  //
                      | ((uint32_t)buf_p[body_len + 3] << 0); //
    uint32_t actual = secure_p2p_crc32(buf_p, body_len);
    if ( expected != actual )
    {
        return secfalse;
    }
    *len_p = body_len;
    return sectrue;
}

void secure_p2p_frame_build_sw(uint16_t sw, uint8_t* out_p, uint32_t* out_len_p, uint32_t capacity)
{
    if ( out_p == NULL || out_len_p == NULL )
    {
        return;
    }
    if ( capacity < 2 + SECURE_P2P_CRC32_LEN )
    {
        *out_len_p = 0;
        return;
    }
    out_p[0] = (uint8_t)(sw >> 8);
    out_p[1] = (uint8_t)(sw & 0xFF);
    *out_len_p = 2;
    if ( secure_p2p_frame_seal(out_p, out_len_p, capacity) != sectrue )
    {
        *out_len_p = 0;
    }
}
