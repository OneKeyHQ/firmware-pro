#pragma once

#include <stdint.h>

// NFC HAL types shared by the driver and trezorhal/nfc.

// ########################################################################
// defines

typedef enum
{
    NFC_RESULT_OK = 0,
    NFC_RESULT_ErrOther,
    NFC_RESULT_ErrGeneral,
    NFC_RESULT_ErrState,
    NFC_RESULT_ErrTimeout,
} NfcResult_t;

// NFC role for nfc_mode_ctrl / pn532_mode_ctrl. mode_ctrl only applies chip
// configuration - it never starts RF discovery.
typedef enum
{
    NFC_MODE_OFF = 0, // chip powered, no mode configured
    NFC_MODE_READER,  // reader config loaded
    NFC_MODE_TAG,     // tag (card emulation) config loaded
} NfcMode_t;

// Activation snapshot for the reader-session sub-API.
typedef enum
{
    NFC_PROTO_UNDETERMINED = 0,
    NFC_PROTO_ISODEP, // ISO 14443-4 (T=CL)
    NFC_PROTO_T2T,    // Type 2 Tag (storage)
} NfcProto_t;

typedef struct
{
    NfcProto_t protocol;
    uint8_t atqa[2]; // Type A SENS_RES (ATQA)
    uint8_t atqa_len;
    uint8_t sak; // Type A SEL_RES (SAK)
    uint8_t sak_valid;
    uint8_t uid[10]; // NFCID1
    uint8_t uid_len;
    uint8_t ats[32]; // ATS from T0 (TL stripped)
    uint8_t ats_len;
} NfcActivationInfo_t;
