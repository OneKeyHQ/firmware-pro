#include STM32_HAL_H

#include <string.h>

#include "systick.h"

#include "nfc_defines.h"
#include "pn532_defines.h"
#include "pn532_hal.h"
#include "pn532_frame.h"
#include "pn532_functions.h"
#include "pn532_spi.h"
#include "pn532_stub.h"
#include "pn532.h"

// ########################################################################
// State machine
//
// Single source of truth for driver state. Every public API call asserts
// its allowed entry state(s) and updates on exit. State transition table
// is in pn532.h.

typedef enum
{
    PN532_S_UNINIT = 0,
    PN532_S_HW_READY,          // pn532_init() done - interface registered
    PN532_S_POWERED,           // pn532_pwr_ctl(true) - PowerOn + SAMConfig + SetParameters done
    PN532_S_READER_CONFIGURED, // mode_ctrl(READER, true) - state-only (no hw command)
    PN532_S_READER_FIELD_ON,   // rsession_field(true) - ready to poll (no hw command)
    PN532_S_POLL_ACTIVE,       // rsession_activate found a tag, g_tg cached
    PN532_S_TAG_CONFIGURED,    // mode_ctrl(TAG, true) - state-only
} pn532_state_t;

static pn532_state_t g_state = PN532_S_UNINIT;
static uint8_t g_tg = 1; // tag number from InListPassiveTarget; MaxTg=1 -> always 1

// ########################################################################
// Board glue: SPI bus interface

static pn532_spi_t* g_spi = NULL;
static pn532_stub_t* g_stub = NULL;

static bool pn532_bus_init(void)
{
    g_stub->init();
    g_spi->spi_init();
    g_spi->chip_select(false);
    return true;
}

static bool pn532_bus_deinit(void)
{
    g_spi->spi_deinit();
    return true;
}

static void pn532_reset_ctl(bool state)
{
    // state: true = inactive (High), false = active (Low/Reset)
    g_stub->chip_reset_ctl(state);
}

static void pn532_chip_sel_ctl(bool state)
{
    // state: true = select (Low), false = deselect (High)
    g_spi->chip_select(state);
}

// Exact-length frame read for a PN532_SPI_DATAREAD transaction. A full
// PN532_FRAME_RAW_MAX burst is not safe: a long read keeps clocking until the
// chip's next response is ready, shifts it out inside the same CS transaction as
// padding, and the following RDY wait then times out. So read the header, derive
// the frame length and read exactly that. Bytes beyond the frame stay zero, so
// pn532_frame_read() parses the buffer unchanged.
static void pn532_read_frame_exact(uint8_t* r_data, uint16_t r_count)
{
    memset(r_data, 0, r_count);

    // preamble + start code: 00 00 FF (some firmwares drop the preamble)
    g_spi->read(r_data, 3);
    uint16_t hdr;
    if ( r_data[0] == 0x00 && r_data[1] == 0xFF )
        hdr = 2;
    else if ( r_data[1] == 0x00 && r_data[2] == 0xFF )
        hdr = 3;
    else
    {
        // not a frame start - drain the rest and let the parser report it
        g_spi->read(r_data + 3, r_count - 3);
        return;
    }
    uint16_t idx = 3;
    while ( idx < hdr + 2 )
    {
        g_spi->read(r_data + idx, 1);
        idx++;
    }

    uint8_t len = r_data[hdr];
    uint8_t lcs = r_data[hdr + 1];
    uint32_t body; // everything after LCS: (TFI..DCS) + postamble
    if ( len == 0x00 && lcs == 0xFF )
        body = 1; // ACK
    else if ( len == 0xFF && lcs == 0x00 )
        body = 1; // NACK
    else if ( len == 0xFF && lcs == 0xFF )
    {
        // extended frame: LENM LENL LCS follow
        g_spi->read(r_data + idx, 3);
        body = (uint32_t)(((uint16_t)r_data[idx] << 8) | r_data[idx + 1]) + 2;
        idx += 3;
    }
    else
        body = (uint32_t)len + 2;

    if ( body > (uint32_t)(r_count - idx) )
        body = r_count - idx;
    if ( body > 0 )
        g_spi->read(r_data + idx, (uint16_t)body);
}

static bool pn532_rw(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count)
{
    g_spi->chip_select(true);
    // UM0701-02 section 6.3.4 (handshake) + Fig 49 (wake-up sequence): the PN532
    // firmware needs time to service the transaction after CS assert.
    dwt_delay_ms(1);

    if ( w_count > 0 )
    {
        g_spi->write(w_data, w_count);
    }
    if ( r_count > 0 )
    {
        if ( w_count == 1 && w_data[0] == PN532_SPI_DATAREAD && r_count >= 8 )
            pn532_read_frame_exact(r_data, r_count);
        else
            g_spi->read(r_data, r_count); // status read etc.
    }

    g_spi->chip_select(false);
    // Let the PN532 firmware finish the transaction before the next CS cycle.
    dwt_delay_ms(1);

    return true;
}

static void pn532_delay_ms(uint16_t timeout)
{
    dwt_delay_ms(timeout);
}

static const PN532_INTERFACE pn532_board_interface = {
    .bus_init = pn532_bus_init,
    .bus_deinit = pn532_bus_deinit,
    .reset_ctl = pn532_reset_ctl,
    .chip_sel_ctl = pn532_chip_sel_ctl,
    .rw = pn532_rw,
    .delay_ms = pn532_delay_ms,
};

// ########################################################################
// Private helpers

// Best-effort session cleanup before a state downgrade. Ignores return
// values - used from mode_ctrl(off) and pwr_ctl(false) paths where we
// can't meaningfully retry.
static void pn532_session_cleanup(void)
{
    if ( g_state == PN532_S_POLL_ACTIVE )
    {
        uint8_t status = 0;
        (void)InRelease(g_tg, &status);
    }
}

// ########################################################################
// Public API

bool pn532_init(void)
{
    if ( g_state >= PN532_S_HW_READY )
        return true; // idempotent

    g_spi = get_spi_controller();
    g_stub = get_stub_controller();

    // Register the board interface with the PN532 library. Actual GPIO +
    // SPI bring-up happens inside pn532_hal_power_on() during pwr_ctl(true).
    if ( !PN532_InterfaceSetup(&pn532_board_interface) )
        return false;

    // Claim the control pins early and hold the chip in reset until a flow
    // actually powers the field on.
    g_stub->init();
    g_stub->chip_reset_ctl(false);

    g_state = PN532_S_HW_READY;
    return true;
}

NfcResult_t pn532_pwr_ctl(bool on)
{
    if ( on )
    {
        if ( g_state < PN532_S_HW_READY )
            return NFC_RESULT_ErrState;
        if ( g_state >= PN532_S_POWERED )
            return NFC_RESULT_OK; // already powered

        // 1. Bring the chip out of reset + init the SPI bus.
        if ( !pn532_hal_power_on() )
        {
            return NFC_RESULT_ErrGeneral;
        }

        // From here the chip is out of reset with the SPI bus up. Any config
        // failure below must undo that (power_off) before returning, otherwise
        // g_state stays HW_READY and a later pn532_pwr_ctl(false) would
        // early-return "already off" and leave the chip energized with a live
        // RF field.

        // 2. SAMConfiguration(Normal, 20 x 50ms = 1s internal timeout, IRQ enabled).
        if ( !SAMConfiguration(PN532_SAM_Normal, 0x14, true) )
        {
            pn532_hal_power_off();
            return NFC_RESULT_ErrGeneral;
        }

        // 3. SetParameters(AUTOMATIC_ATR_RES | AUTOMATIC_RATS | ISO14443_4_PICC).
        //    - ISO14443_4_PICC is required by TgInitAsTarget PICCOnly mode
        //      (UM0701-02 section 7.3.14 Syntax Error Conditions).
        //    - AUTOMATIC_RATS (default after power-on) must be re-enabled
        //      here: SetParameters overwrites ALL bits, so once we pass a
        //      value without bit 4, InListPassiveTarget stops doing RATS
        //      and leaves ISO-DEP cards in Layer 3 - the next
        //      InDataExchange then fails with application-level error 7F.
        //    - AUTOMATIC_ATR_RES kept at default to preserve any DEP use.
        if ( !SetParameters(
                 PN532_PARAM_AUTOMATIC_ATR_RES | PN532_PARAM_AUTOMATIC_RATS | PN532_PARAM_ISO14443_4_PICC
             ) )
        {
            pn532_hal_power_off();
            return NFC_RESULT_ErrGeneral;
        }

        // 4. RFConfiguration(MaxRetries): MxRtyATR=0xFF (chip default),
        //    MxRtyPSL=0x01, MxRtyPassiveActivation=0x02. The chip default
        //    for passive activation is infinite retries, which makes
        //    InListPassiveTarget block until a card appears; nfc_poll_card() is
        //    a short probe driven by a ~1 Hz UI loop, so discovery must come
        //    back promptly when the field is empty.
        uint8_t max_retries[3] = {0xFF, 0x01, 0x02};
        if ( !RFConfiguration(PN532_RFConfiguration_CfgItem_MaxRetries, max_retries, sizeof(max_retries)) )
        {
            pn532_hal_power_off();
            return NFC_RESULT_ErrGeneral;
        }

        g_state = PN532_S_POWERED;
        return NFC_RESULT_OK;
    }

    // on == false
    if ( g_state < PN532_S_POWERED )
        return NFC_RESULT_OK; // already off

    // Drop any active session back to POWERED first.
    if ( g_state > PN532_S_POWERED )
    {
        pn532_session_cleanup();
        g_state = PN532_S_POWERED;
    }

    pn532_hal_power_off();
    g_state = PN532_S_HW_READY;
    return NFC_RESULT_OK;
}

NfcResult_t pn532_mode_ctrl(NfcMode_t mode, bool on)
{
    if ( g_state < PN532_S_POWERED )
        return NFC_RESULT_ErrState;

    // "off" branch: deactivate any active session and drop to POWERED.
    // Idempotent regardless of the requested mode.
    if ( !on )
    {
        if ( g_state == PN532_S_POWERED )
            return NFC_RESULT_OK;
        pn532_session_cleanup();
        g_state = PN532_S_POWERED;
        (void)mode;
        return NFC_RESULT_OK;
    }

    switch ( mode )
    {
    case NFC_MODE_OFF:
        if ( g_state == PN532_S_POWERED )
            return NFC_RESULT_OK;
        pn532_session_cleanup();
        g_state = PN532_S_POWERED;
        return NFC_RESULT_OK;

    case NFC_MODE_READER:
        if ( g_state == PN532_S_READER_CONFIGURED || g_state == PN532_S_POLL_ACTIVE )
            return NFC_RESULT_OK; // already in this mode

        // If currently in a TAG mode (configured or listening), drop
        // back to POWERED before reconfiguring.
        if ( g_state != PN532_S_POWERED )
        {
            pn532_session_cleanup();
            g_state = PN532_S_POWERED;
        }

        // PN532 is reader-capable after SAMConfiguration - no hardware
        // command is needed to enter reader mode.
        g_state = PN532_S_READER_CONFIGURED;
        return NFC_RESULT_OK;

    case NFC_MODE_TAG:
        if ( g_state == PN532_S_TAG_CONFIGURED )
            return NFC_RESULT_OK;

        if ( g_state != PN532_S_POWERED )
        {
            pn532_session_cleanup();
            g_state = PN532_S_POWERED;
        }

        // Like READER: no hardware command here. The caller runs TgInitAsTarget.
        g_state = PN532_S_TAG_CONFIGURED;
        return NFC_RESULT_OK;
    }

    return NFC_RESULT_ErrGeneral;
}

// ########################################################################
// reader-session sub-API (see pn532.h for the state contract + limits)
//
// Built on InListPassiveTarget (poll) + InDataExchange (transceive) +
// InRelease.

static void
pn532_fill_activation_info_a(const PN532_InListPassiveTarget_Results* r, NfcActivationInfo_t* info)
{
    memset(info, 0, sizeof(*info));

    const uint8_t* td = r->TargetData;
    uint8_t len = r->TargetData_len;
    // TypeA target data (UM0701-02 section 7.3.5):
    //   Tg(1) SENS_RES(2) SEL_RES(1) NFCIDLength(1) NFCID1(n) [ATS(TL bytes)]
    if ( len < 5 )
    {
        info->protocol = NFC_PROTO_UNDETERMINED;
        return;
    }

    info->atqa[0] = td[1];
    info->atqa[1] = td[2];
    info->atqa_len = 2;
    info->sak = td[3];
    info->sak_valid = 1;

    uint8_t id_len = td[4];
    uint8_t id_copy = (id_len > sizeof(info->uid)) ? sizeof(info->uid) : id_len;
    if ( (uint16_t)5 + id_copy <= len )
    {
        memcpy(info->uid, &td[5], id_copy);
        info->uid_len = id_copy;
    }

    // uint16_t: id_len is the wire-controlled NFCIDLength byte (up to 255),
    // so 5 + id_len must not wrap a uint8_t or ats_off could alias back into
    // the SENS_RES/SAK bytes and misparse them as an ATS.
    uint16_t ats_off = 5 + id_len; // ATS follows the full on-wire NFCID1
    if ( ats_off < len )
    {
        // On-wire ATS is TL-prefixed; info->ats carries it from T0 (TL
        // stripped) per the NfcActivationInfo_t convention.
        uint8_t tl = td[ats_off]; // ATS TL (length incl. itself)
        uint8_t avail = len - ats_off;
        if ( tl > avail )
            tl = avail;
        if ( tl > 1 )
        {
            uint8_t n = tl - 1;
            if ( n > sizeof(info->ats) )
                n = sizeof(info->ats);
            memcpy(info->ats, &td[ats_off + 1], n);
            info->ats_len = n;
        }
    }

    info->protocol = (info->ats_len > 0 || (info->sak & 0x20)) ? NFC_PROTO_ISODEP : NFC_PROTO_T2T;
}

NfcResult_t pn532_rsession_field(bool on)
{
    if ( on )
    {
        if ( g_state == PN532_S_READER_FIELD_ON || g_state == PN532_S_POLL_ACTIVE )
            return NFC_RESULT_OK;
        if ( g_state != PN532_S_READER_CONFIGURED )
            return NFC_RESULT_ErrState;
        g_state = PN532_S_READER_FIELD_ON;
        return NFC_RESULT_OK;
    }

    if ( g_state != PN532_S_READER_FIELD_ON && g_state != PN532_S_POLL_ACTIVE )
        return NFC_RESULT_OK;
    if ( g_state == PN532_S_POLL_ACTIVE )
    {
        uint8_t status = 0;
        (void)InRelease(g_tg, &status);
    }
    g_state = PN532_S_READER_CONFIGURED;
    return NFC_RESULT_OK;
}

NfcResult_t pn532_rsession_activate(NfcActivationInfo_t* info, uint32_t timeout_ms)
{
    if ( info == NULL || timeout_ms == 0 )
        return NFC_RESULT_ErrGeneral;
    if ( g_state != PN532_S_READER_FIELD_ON )
        return NFC_RESULT_ErrState;

    PN532_InListPassiveTarget_Params params = {0};
    PN532_InListPassiveTarget_Results results;
    uint32_t t_start = HAL_GetTick();
    bool found = false;

    params.MaxTg = 1;
    do
    {
        memset(&results, 0, sizeof(results));
        found = InListPassiveTarget(params, &results) && results.NbTg > 0;
    }
    while ( !found && (HAL_GetTick() - t_start) < timeout_ms );

    if ( !found )
        return NFC_RESULT_ErrTimeout;

    g_tg = (results.TargetData_len > 0) ? results.TargetData[0] : 1;
    pn532_fill_activation_info_a(&results, info);
    g_state = PN532_S_POLL_ACTIVE;
    return NFC_RESULT_OK;
}

NfcResult_t pn532_rsession_transceive(
    const uint8_t* cmd, uint16_t cmd_len, uint8_t* resp, uint16_t* resp_len, uint32_t timeout_ms
)
{
    if ( cmd == NULL || resp == NULL || resp_len == NULL )
        return NFC_RESULT_ErrGeneral;
    if ( timeout_ms == 0 || cmd_len == 0 || *resp_len == 0 )
        return NFC_RESULT_ErrGeneral;
    if ( cmd_len > PN532_DATA_MAX_LEN - 1 ) // InDataExchange payload: Tg + 262 max
        return NFC_RESULT_ErrGeneral;
    if ( g_state != PN532_S_POLL_ACTIVE )
        return NFC_RESULT_ErrState;

    // One InDataExchange carries the whole command (the chip does T=CL
    // I-block chaining to the card per FSC). A response larger than one
    // host frame comes back chunked: status bit 6 (MI) set means more data,
    // fetched with data-less InDataExchange continuations (UM0701-02
    // sec 7.4.5). Status bits 0-5 are the error code.
    uint8_t status = 0;
    uint16_t total = 0;
    uint32_t t_start = HAL_GetTick();
    const uint8_t* chunk_out = cmd;
    uint16_t chunk_out_len = cmd_len;

    do
    {
        uint32_t elapsed = HAL_GetTick() - t_start;
        if ( elapsed >= timeout_ms )
            break;

        uint16_t got = *resp_len - total;
        if ( !InDataExchange_ADV(
                 g_tg, (uint8_t*)chunk_out, chunk_out_len, &status, &resp[total], &got, timeout_ms - elapsed
             ) ||
             (status & 0x3F) != 0 )
        {
            uint8_t rel_status = 0;
            (void)InRelease(g_tg, &rel_status);
            g_state = PN532_S_READER_CONFIGURED;
            return NFC_RESULT_ErrState;
        }
        total += got;

        chunk_out = NULL; // continuations carry no data
        chunk_out_len = 0;
    }
    while ( (status & 0x40) != 0 && total < *resp_len );

    if ( (status & 0x40) != 0 )
    {
        // Deadline hit or response capacity exceeded mid-chain: the chip
        // still holds pending chunks, so the session state is unknown.
        uint8_t rel_status = 0;
        (void)InRelease(g_tg, &rel_status);
        g_state = PN532_S_READER_CONFIGURED;
        return NFC_RESULT_ErrState;
    }

    *resp_len = total;
    return NFC_RESULT_OK;
}
