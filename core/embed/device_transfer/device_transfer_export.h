#pragma once

// Device-to-device transfer, exporter side (NFC tag / ISO-DEP target).
//
// Runs the secure_p2p *card* role of the transfer protocol v4
// (secure_p2p/transfer) over PN532 card emulation:
// SELECT -> EXCHANGE -> SCP03 0x33 -> AUTH_PUT / AUTH_GET (mutual device
// certificate authentication; the SE verifies the reader with 84 F5 00 0A)
// -> EXPORT, or BATCH_KEYS / BATCH_GET for a four-SE reader (the SE seals the
// mnemonic into one envelope per receiving SE, each after a fresh F5/0A)
// -> RELEASE.
//
// Which readers are accepted is decided by the CA slots in the SE. The exporter
// takes the verified user-entered phrase for E2/09; it never reads the stored
// mnemonic.

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    DEVICE_TRANSFER_EXPORT_OK = 0,
    DEVICE_TRANSFER_EXPORT_ERR_PARAM = 1,     // bad arguments / session init
    DEVICE_TRANSFER_EXPORT_ERR_POWER = 2,     // pn532_pwr_ctl(true) failed
    DEVICE_TRANSFER_EXPORT_ERR_MODE = 3,      // mode_ctrl(TAG) failed
    DEVICE_TRANSFER_EXPORT_ERR_NO_READER = 4, // no transfer peer within listen_timeout_ms
    DEVICE_TRANSFER_EXPORT_ERR_LINK = 5,      // link lost / turn timeout mid-session
    DEVICE_TRANSFER_EXPORT_ERR_NFC = 6,       // other NFC driver error
    DEVICE_TRANSFER_EXPORT_ERR_PROTOCOL = 8,  // session ended on a failure SW (see last_state / export_sw)
    DEVICE_TRANSFER_EXPORT_ERR_SE = 9,        // no usable device certificate in the SE
    DEVICE_TRANSFER_EXPORT_CANCELLED = 10,    // closed while the session was running
} device_transfer_export_result_t;

typedef struct
{
    uint32_t frames_in;  // L1 frames received from the reader
    uint32_t frames_out; // L1 frames sent back
    uint16_t export_sw;  // SW the export callback reported (0 = never failed there)
    uint8_t last_state;  // SecureP2p_TransferCardState_t after the last frame
    bool exported;       // the envelope was handed over and wiped
    bool ended_clean;    // reader sent RELEASE and we answered it
} device_transfer_export_stats_t;

// Asynchronous exporter driven by the UI. -1 means pending.
// Takes verified user-entered BIP39 text (1..240 bytes, no NUL). The SE seals
// it with 84 E2 00 09, once per receiving SE; the adapter wipes its copy once
// every envelope is sealed, on any failure, and on close.
// Only one session may own the NFC chip. Poll at least every 10 ms.
// A reader whose frames fail the L1 CRC, or one that leaves before a valid
// frame, is ignored; all of that waiting shares listen_timeout_ms.
// A success result relies on the receiver sending RELEASE only after all its
// SE imports succeed. A missing RELEASE leaves the receiver's import outcome unconfirmed.
int device_transfer_export_start(
    const uint8_t* mnemonic, uint16_t mnemonic_len, uint32_t listen_timeout_ms, uint32_t turn_timeout_ms
);
int device_transfer_export_poll(device_transfer_export_stats_t* stats);
void device_transfer_export_close(void);
