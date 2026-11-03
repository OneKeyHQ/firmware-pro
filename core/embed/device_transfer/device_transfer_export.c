#include "device_transfer_export.h"

#include STM32_HAL_H

#include <string.h>

#include "common.h" // hal_ticks_ms
#include "memzero.h"
#include "nfc.h"      // pn532_* dual-mode driver + nfc_* aliases
#include "se_thd89.h" // se_read_certificate / se_sign_digest_with_write_key / se_verify_transfer_peer / ...
#include "systick.h"  // dwt_delay_ms
#include "thd89.h"    // thd89_last_error

#include <secure_p2p_defines.h>
#include <secure_p2p_frame.h>
#include <secure_p2p_handshake.h>
#include <secure_p2p_transfer.h>

#include "device_transfer_io.h"
#include "device_transfer_se.h"
#include "pn532_defines.h"

#define EXPORTER_FINAL_PUSH_TIMEOUT_MS 2000
#define EXPORTER_STATUS_WINDOW_MS      60000

// Set from the first F5/0A attempt on: the SE may hold V, so the session ends
// with F5/0B. Only one exporter session runs at a time.
static bool se_peer_armed;

static struct
{
    SecureP2p_TransferCardSession_t session;
    SecureP2p_HandshakeCfg_t cfg;
    SecureP2p_TransferCardApp_t app;
    device_transfer_export_stats_t stats;
    uint8_t cert[SECURE_P2P_HANDSHAKE_CERT_MAX_LEN];
    uint8_t mnemonic[DEVICE_TRANSFER_MNEMONIC_MAX_LEN];
    uint16_t mnemonic_len;
    uint32_t listen_started;
    uint32_t listen_ms;
    uint32_t turn_ms;
    int result;
    uint8_t command;
    bool active;
    bool final_reply;
} exporter = {.result = DEVICE_TRANSFER_EXPORT_ERR_PARAM};

// ########################################################################
// Card-side callbacks

// cfg.sign: the handshake hands us the finished 32-byte digest
// m_B = SHA-256("onekey-p2p-auth-v1" || "B" || T). The SE signs exactly
// those bytes with the certificate key, adding neither hash nor prefix.
static secbool_t exporter_sign(void* ctx_p, const uint8_t digest[32], uint8_t out_sig[64])
{
    (void)ctx_p;
    return se_sign_digest_with_write_key(digest, out_sig);
}

static void exporter_status(void* ctx_p, uint8_t* ready_p, uint32_t* remaining_ms_p)
{
    (void)ctx_p;
    *ready_p = 1;
    *remaining_ms_p = EXPORTER_STATUS_WINDOW_MS;
}

// 84 F5 00 0A: the SE verifies the reader's certificate under the CA the
// reader named in SELECT and its sigma over T, and arms V for one export.
// Used as app.verify_peer (gate before AUTH_B is signed) and again in front
// of every export, since each E2/09 consumes V.
static secbool_t se_verify_peer(const uint8_t* proof_p, uint32_t proof_len)
{
    if ( proof_len > SE_TRANSFER_PEER_PROOF_MAX_LEN )
    {
        return secfalse;
    }
    // Set before sending: if the 9000 is lost on the way back, V is armed in
    // the SE although this call fails, and close must still send F5/0B.
    se_peer_armed = true;
    if ( se_verify_transfer_peer(proof_p, (uint16_t)proof_len) != sectrue )
    {
        return secfalse;
    }
    return sectrue;
}

// Clears V when the session ends; the result is irrelevant because the SE clears
// it even when it answers an error.
static void se_disarm_peer(void)
{
    if ( se_peer_armed )
    {
        (void)se_cancel_transfer();
        se_peer_armed = false;
    }
}

static void exporter_wipe_mnemonic(void)
{
    memzero(exporter.mnemonic, sizeof(exporter.mnemonic));
    exporter.mnemonic_len = 0;
}

static secbool_t exporter_verify_peer(void* ctx_p, const uint8_t* proof_p, uint32_t proof_len)
{
    (void)ctx_p;
    if ( se_verify_peer(proof_p, proof_len) != sectrue )
    {
        return secfalse;
    }
    return sectrue;
}

// The phrase was entered by the user and checked against this wallet.
// After F5/0A, E2/09 verifies it again in the unlocked SE and seals it for R.
// This path neither reads the stored mnemonic nor changes the persistent
// export switch. The phrase stays in RAM for the remaining envelopes of a
// four-envelope reader; export_poll wipes it once the session has exported
// or ended, and any failure here wipes it at once.
static secbool_t export_verified_mnemonic(
    void* ctx_p, const uint8_t* proof_p, uint32_t proof_len, const uint8_t r_xy[64], uint8_t* env_p,
    uint32_t* env_len_p, uint32_t env_cap, uint16_t* err_sw_p
)
{
    (void)ctx_p;
    uint16_t env_len = (uint16_t)env_cap;
    if ( se_verify_peer(proof_p, proof_len) != sectrue )
    {
        exporter_wipe_mnemonic();
        *err_sw_p = SECURE_P2P_TRANSFER_SW_SE_FAILED;
        exporter.stats.export_sw = *err_sw_p;
        return secfalse;
    }
    thd89_clear_last_error();
    secbool result =
        device_transfer_seal_mnemonic(exporter.mnemonic, exporter.mnemonic_len, r_xy, env_p, &env_len);
    if ( result != sectrue )
    {
        exporter_wipe_mnemonic();
        // sw 0000/9000 here means the SE answered but the ENV failed the local
        // format check (or the input copy was already wiped).
        *err_sw_p = thd89_last_error() == 0x6A80 ? SECURE_P2P_TRANSFER_SW_WRONG_DATA
                                                 : SECURE_P2P_TRANSFER_SW_SE_FAILED;
        exporter.stats.export_sw = *err_sw_p;
        return secfalse;
    }
    *env_len_p = env_len;
    return sectrue;
}

void device_transfer_export_close(void)
{
    if ( exporter.active )
    {
        device_transfer_io_close();
        (void)nfc_mode_ctrl(NFC_MODE_TAG, false);
        (void)pn532_pwr_ctl(false);
        exporter.result = DEVICE_TRANSFER_EXPORT_CANCELLED;
    }
    se_disarm_peer();
    secure_p2p_transfer_card_destroy(&exporter.session);
    exporter_wipe_mnemonic();
    memzero(exporter.cert, sizeof(exporter.cert));
    exporter.active = false;
}

static int export_finish(int result)
{
    device_transfer_export_close();
    exporter.result = result;
    return result;
}

// Until a frame passes the L1 CRC no peer exists: the reader may be a foreign
// one, such as a Seed Card poll that leaves and comes back. It gets no reply
// and no failure, and all waiting shares one listen window.
static bool exporter_listening(void)
{
    return exporter.stats.frames_in == 0;
}

static uint32_t exporter_listen_remaining(void)
{
    uint32_t elapsed = hal_ticks_ms() - exporter.listen_started;
    return elapsed < exporter.listen_ms ? exporter.listen_ms - elapsed : 0;
}

// TgInitAsTarget: ISO 14443-4 PICC emulation, PICCOnly + passive only.
static bool exporter_start_target(uint32_t timeout_ms)
{
    uint8_t params[37] = {0x05, 0x04, 0x00};
    uint32_t uid[3] = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()};
    memcpy(params + 3, uid, 3);
    params[6] = 0x20;
    memcpy(params + 25, uid, PN532_NFCID3_LEN);
    exporter.command = PN532_COMMAND_TGINITASTARGET;
    return device_transfer_io_start(exporter.command, params, sizeof(params), timeout_ms);
}

static void exporter_relisten(void)
{
    // Abort the pending command and give the chip a moment before restarting.
    device_transfer_io_close();
    dwt_delay_ms(5);
    uint32_t remaining = exporter_listen_remaining();
    if ( remaining == 0 )
        export_finish(DEVICE_TRANSFER_EXPORT_ERR_NO_READER);
    else if ( !exporter_start_target(remaining) )
        export_finish(DEVICE_TRANSFER_EXPORT_ERR_NFC);
}

int device_transfer_export_start(
    const uint8_t* mnemonic, uint16_t mnemonic_len, uint32_t listen_timeout_ms, uint32_t turn_timeout_ms
)
{
    if ( exporter.active || mnemonic == NULL || mnemonic_len == 0 ||
         mnemonic_len > sizeof(exporter.mnemonic) || memchr(mnemonic, '\0', mnemonic_len) != NULL ||
         listen_timeout_ms == 0 || turn_timeout_ms == 0 )
        return DEVICE_TRANSFER_EXPORT_ERR_PARAM;
    memzero(&exporter, sizeof(exporter));
    exporter.result = -1;
    // F5/02 and F5/04 below keep V, so drop any V a previous session failed to
    // cancel before this one starts.
    (void)se_cancel_transfer();
    uint16_t cert_len = sizeof(exporter.cert);
    if ( se_read_certificate(exporter.cert, &cert_len) != sectrue || cert_len == 0 ||
         cert_len > sizeof(exporter.cert) )
        return export_finish(DEVICE_TRANSFER_EXPORT_ERR_SE);
    memcpy(exporter.mnemonic, mnemonic, mnemonic_len);
    exporter.mnemonic_len = mnemonic_len;
    exporter.cfg = (SecureP2p_HandshakeCfg_t){
        .cert_p = exporter.cert,
        .cert_len = cert_len,
        .ca_id = SECURE_P2P_IDENTITY_CA_ID_PRO,
        .sign = exporter_sign,
    };
    exporter.app = (SecureP2p_TransferCardApp_t
    ){.status = exporter_status,
      .verify_peer = exporter_verify_peer,
      .export_envelope = export_verified_mnemonic};
    if ( secure_p2p_transfer_card_init(&exporter.session, &exporter.cfg, &exporter.app) != sectrue )
        return export_finish(DEVICE_TRANSFER_EXPORT_ERR_PARAM);
    exporter.active = true;
    exporter.turn_ms = turn_timeout_ms;
    (void)pn532_pwr_ctl(false);
    if ( pn532_pwr_ctl(true) != NFC_RESULT_OK )
        return export_finish(DEVICE_TRANSFER_EXPORT_ERR_POWER);
    if ( nfc_mode_ctrl(NFC_MODE_TAG, true) != NFC_RESULT_OK )
        return export_finish(DEVICE_TRANSFER_EXPORT_ERR_MODE);

    exporter.listen_started = hal_ticks_ms();
    exporter.listen_ms = listen_timeout_ms;
    if ( !exporter_start_target(listen_timeout_ms) )
        return export_finish(DEVICE_TRANSFER_EXPORT_ERR_NFC);
    return 0;
}

int device_transfer_export_poll(device_transfer_export_stats_t* stats)
{
    if ( !exporter.active )
    {
        if ( stats )
            *stats = exporter.stats;
        return exporter.result;
    }
    uint8_t data[PN532_FRAME_DATA_MAX] = {0};
    uint16_t length = sizeof(data);
    device_transfer_io_result_t io_result = device_transfer_io_poll(data, &length);
    if ( io_result == DEVICE_TRANSFER_IO_WAIT )
        goto done;
    if ( io_result != DEVICE_TRANSFER_IO_DONE )
    {
        if ( exporter_listening() && exporter.command == PN532_COMMAND_TGGETDATA &&
             io_result != DEVICE_TRANSFER_IO_TIMEOUT )
        {
            exporter_relisten();
            goto done;
        }
        export_finish(
            io_result == DEVICE_TRANSFER_IO_TIMEOUT && !exporter.stats.frames_in
                ? DEVICE_TRANSFER_EXPORT_ERR_NO_READER
            : exporter.stats.frames_in ? DEVICE_TRANSFER_EXPORT_ERR_LINK
                                       : DEVICE_TRANSFER_EXPORT_ERR_NFC
        );
        goto done;
    }
    if ( length == 0 || (exporter.command != PN532_COMMAND_TGINITASTARGET && data[0] != 0) )
    {
        if ( exporter_listening() && exporter.command == PN532_COMMAND_TGGETDATA )
        {
            exporter_relisten();
            goto done;
        }
        export_finish(
            exporter.stats.frames_in ? DEVICE_TRANSFER_EXPORT_ERR_LINK : DEVICE_TRANSFER_EXPORT_ERR_NFC
        );
        goto done;
    }
    if ( exporter.command == PN532_COMMAND_TGSETDATA )
    {
        exporter.stats.frames_out++;
        if ( exporter.final_reply )
        {
            export_finish(
                exporter.stats.ended_clean ? DEVICE_TRANSFER_EXPORT_OK : DEVICE_TRANSFER_EXPORT_ERR_PROTOCOL
            );
            goto done;
        }
    }
    else if ( exporter.command == PN532_COMMAND_TGGETDATA )
    {
        uint32_t frame_length = length - 1;
        if ( secure_p2p_frame_verify(data + 1, &frame_length) != sectrue )
        {
            // Strip a PN532 CID only when the remaining frame's CRC verifies.
            frame_length = length >= 2 ? length - 2 : 0;
            if ( length < 2 || secure_p2p_frame_verify(data + 2, &frame_length) != sectrue )
            {
                if ( exporter_listening() )
                    goto next_frame;
                export_finish(DEVICE_TRANSFER_EXPORT_ERR_PROTOCOL);
                goto done;
            }
            length--;
            memmove(data + 1, data + 2, length - 1);
        }
        if ( exporter.stats.frames_in && length >= 3 && data[1] == SECURE_P2P_CLA_APP &&
             data[2] == SECURE_P2P_TRANSFER_INS_SELECT )
        {
            export_finish(DEVICE_TRANSFER_EXPORT_ERR_PROTOCOL);
            goto done;
        }
        exporter.stats.frames_in++;
        uint8_t reply[SECURE_P2P_FRAME_MAX] = {0};
        uint32_t reply_len = 0;
        secbool_t keep = secure_p2p_transfer_card_handle_frame(
            &exporter.session, data + 1, length - 1, reply, &reply_len, sizeof(reply)
        );
        exporter.stats.last_state = (uint8_t)exporter.session.state;
        if ( exporter.session.state == SECURE_P2P_TRANSFER_CARD_STATE_EXPORTED )
            exporter.stats.exported = true;
        exporter.final_reply = keep != sectrue;
        // Every negotiated envelope is sealed, or the session is over.
        if ( exporter.stats.exported || exporter.final_reply )
            exporter_wipe_mnemonic();
        // ENDED also covers failures. A successful export must precede an
        // authenticated RELEASE with SW 9000, and its reply must reach the chip.
        // The receiver must commit all SE imports before sending RELEASE.
        // No RELEASE confirms neither import success nor import failure.
        exporter.stats.ended_clean = exporter.final_reply && exporter.stats.exported && length >= 3 &&
                                     data[2] == SECURE_P2P_TRANSFER_INS_RELEASE && reply_len >= 6 &&
                                     reply[reply_len - 6] == 0x90 && reply[reply_len - 5] == 0x00;
        exporter.command = PN532_COMMAND_TGSETDATA;
        bool started =
            reply_len > 0 && device_transfer_io_start(
                                 exporter.command, reply, (uint16_t)reply_len,
                                 exporter.final_reply ? EXPORTER_FINAL_PUSH_TIMEOUT_MS : exporter.turn_ms
                             );
        memzero(reply, sizeof(reply));
        if ( !started )
            export_finish(DEVICE_TRANSFER_EXPORT_ERR_NFC);
        goto done;
    }
next_frame:
    exporter.command = PN532_COMMAND_TGGETDATA;
    uint32_t wait_ms = exporter_listening() ? exporter_listen_remaining() : exporter.turn_ms;
    if ( wait_ms == 0 )
        export_finish(DEVICE_TRANSFER_EXPORT_ERR_NO_READER);
    else if ( !device_transfer_io_start(exporter.command, NULL, 0, wait_ms) )
        export_finish(DEVICE_TRANSFER_EXPORT_ERR_NFC);
done:
    memzero(data, sizeof(data));
    if ( stats )
        *stats = exporter.stats;
    return exporter.active ? -1 : exporter.result;
}
