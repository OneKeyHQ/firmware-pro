#pragma once

#include "scp03.h"

// ########################################################################
// public functions

// ########## secure messaging APDU wrap/unwrap ##########

secbool_t __wur secure_channel_scp03_apdu_wrap_command(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* apdu_p, uint32_t apdu_len, uint8_t* out_p,
    uint32_t* out_len_p
);

secbool_t __wur secure_channel_scp03_apdu_unwrap_command(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* apdu_p, uint32_t apdu_len, uint8_t* out_p,
    uint32_t* out_len_p
);

secbool_t __wur secure_channel_scp03_apdu_wrap_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, uint8_t sw1,
    uint8_t sw2, uint8_t* out_p, uint32_t* out_len_p
);

secbool_t __wur secure_channel_scp03_apdu_unwrap_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* response_p, uint32_t response_len,
    uint8_t* data_p, uint32_t* data_len_p, uint8_t* sw1_p, uint8_t* sw2_p
);
