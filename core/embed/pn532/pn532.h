#ifndef _PN532_H_
#define _PN532_H_

#include <stdint.h>
#include <stdbool.h>

#include "nfc_defines.h"
#include "pn532_defines.h"

// Dual-mode (reader / card emulation) PN532 driver, reached through
// pn532_spi.c / pn532_stub.c.
//
// State contract:
//
//   UNINIT --init()--> HW_READY --pwr_ctl(true)--> POWERED
//   POWERED --mode_ctrl(READER,true)--> READER_CONFIGURED
//           +--mode_ctrl(TAG,true)-----> TAG_CONFIGURED
//   READER_CONFIGURED --mode_ctrl(READER,false)--> POWERED
//   TAG_CONFIGURED --mode_ctrl(TAG,false)--> POWERED
//   POWERED --pwr_ctl(false)--> HW_READY

bool pn532_init(void);

// -- Power control --
NfcResult_t pn532_pwr_ctl(bool on);
// on=true:  PowerOn + SAMConfiguration(Normal) + SetParameters(AutomaticATR_RES |
//           AutomaticRATS | ISO14443_4_PICC) + RFConfiguration(MaxRetries).
//           End state: POWERED / no mode configured.
// on=false: tear down any active session, PowerOff. End state: HW_READY.

// -- Mode control (pure configuration; no RF activity) --
NfcResult_t pn532_mode_ctrl(NfcMode_t mode, bool on);
// on=true: state transition only, no hardware command. The caller drives the
//          RF commands afterwards. NFC_MODE_OFF behaves like on=false.
// on=false: release any active session (InRelease if POLL_ACTIVE) and drop to
//           POWERED. The `mode` argument is ignored in the off branch.
// Idempotent; safe to call with either value at any state from POWERED upward.

// -- Reader-session sub-API (decomposed reader flow) --
//
//   READER_CONFIGURED --rsession_field(true)--> READER_FIELD_ON
//   READER_FIELD_ON --rsession_activate() ok--> POLL_ACTIVE
//   POLL_ACTIVE --rsession_transceive--> POLL_ACTIVE
//   POLL_ACTIVE/READER_FIELD_ON --rsession_field(false)--> READER_CONFIGURED

NfcResult_t pn532_rsession_field(bool on);
// PN532 has no persistent-field command (InListPassiveTarget powers the RF
// per poll), so on=true is a state-only READER_CONFIGURED->READER_FIELD_ON
// transition. on=false releases any held tag (InRelease) -> READER_CONFIGURED.

NfcResult_t pn532_rsession_activate(NfcActivationInfo_t* info, uint32_t timeout_ms);
// State READER_FIELD_ON. Poll InListPassiveTarget (Type A) up to timeout_ms; on a
// hit fill *info from the target data (ATQA/SAK/ATS) and cache the tag number.
// Success -> POLL_ACTIVE.

NfcResult_t pn532_rsession_transceive(
    const uint8_t* cmd, uint16_t cmd_len, uint8_t* resp, uint16_t* resp_len, uint32_t timeout_ms
);
// State POLL_ACTIVE. InDataExchange on the active tag; the chip does the
// T=CL I-block chaining per FSC. cmd_len up to 261 (one InDataExchange
// payload); chunked responses (status MI bit) are reassembled with data-less
// continuations, so the full short-APDU envelope passes. timeout_ms bounds
// the whole exchange - slow cards extend via S(WTX), handled by the chip.
// *resp_len is capacity in / actual out. On failure -> READER_CONFIGURED,
// ErrState.

#endif
