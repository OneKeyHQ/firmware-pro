
#include "embed/extmod/trezorobj.h"

#include "manage_card.h"
#include "memzero.h"
#include "nfc.h"

#include "device_transfer_export.h"
#include "device_transfer_se.h"

/// package: trezorio.nfc

/// def pwr_ctrl(on_off: bool) -> bool:
///     """
///     Control NFC power.
///     """
STATIC mp_obj_t mod_trezorio_NFC_pwr_ctrl(mp_obj_t on_off) {
  bool on = mp_obj_is_true(on_off);
  if (!on) {
    // Field down: the next card on the reader need not be the same one, so the
    // secure channel and the detected card generation both go with it.
    manage_card_reset();
  }
  return nfc_pwr_ctl(on) ? mp_const_true : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_NFC_pwr_ctrl_obj,
                                 mod_trezorio_NFC_pwr_ctrl);

/// def poll_card() -> bool:
///     """
///     Poll card.
///     """
STATIC mp_obj_t mod_trezorio_NFC_poll_card(void) {
  return nfc_poll_card() ? mp_const_true : mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_NFC_poll_card_obj,
                                 mod_trezorio_NFC_poll_card);

/// def send_recv(apdu: bytes, safe: bool = False) -> tuple[bytes, bytes]:
///     """
///     Send receive data through NFC.
///     """
STATIC mp_obj_t mod_trezorio_NFC_send_recv(size_t n_args,
                                           const mp_obj_t *args) {
  bool safe = n_args > 1 && args[1] == mp_const_true;
  mp_buffer_info_t apdu = {0};
  mp_get_buffer_raise(args[0], &apdu, MP_BUFFER_READ);

  if (apdu.len > 255) {
    mp_raise_msg(&mp_type_ValueError, "APDU too long");
  }

  uint8_t sw1sw2[2] = {0};
  uint8_t resp[256] = {0};
  uint16_t resp_len = sizeof(resp);

  bool success = manage_card_apdu((uint8_t *)apdu.buf, apdu.len, resp,
                                  &resp_len, sw1sw2, safe);

  mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(mp_obj_new_tuple(2, NULL));

  tuple->items[0] = mp_obj_new_str_copy(&mp_type_bytes, resp, resp_len);
  memzero(resp, sizeof(resp));

  if (!success) {
    sw1sw2[0] = 0x99;
    sw1sw2[1] = 0x99;
  }

  tuple->items[1] = mp_obj_new_str_copy(&mp_type_bytes, sw1sw2, 2);

  return MP_OBJ_FROM_PTR(tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_trezorio_NFC_send_recv_obj, 1, 2,
                                           mod_trezorio_NFC_send_recv);

/// def close_secure_channel() -> None:
///     """
///     Tear down the SCP11/SCP03 session. The next APDU sent with safe=True
///     negotiates a fresh one. Needed where the card drops the session on its
///     own side without the applet being re-selected.
///     """
STATIC mp_obj_t mod_trezorio_NFC_close_secure_channel(void) {
  manage_card_close_secure_channel();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_NFC_close_secure_channel_obj,
                                 mod_trezorio_NFC_close_secure_channel);

/// def p2p_export_start(
///     mnemonic: AnyBytes,
///     listen_timeout_ms: int,
///     turn_timeout_ms: int,
/// ) -> int:
///     """Start one exporter session. Returns 0 on start, otherwise an error.
///     Copies verified user-entered BIP39 text for SE sealing with E2/09.
///     Does not change the SE export switch.
///     """
STATIC mp_obj_t mod_trezorio_NFC_p2p_export_start(size_t n_args,
                                                  const mp_obj_t *args) {
  mp_buffer_info_t mnemonic = {0};
  mp_get_buffer_raise(args[0], &mnemonic, MP_BUFFER_READ);
  mp_int_t listen = trezor_obj_get_int(args[1]);
  mp_int_t turn = trezor_obj_get_int(args[2]);
  if (listen <= 0 || turn <= 0) mp_raise_ValueError("Invalid timeout");
  if (mnemonic.len == 0 || mnemonic.len > DEVICE_TRANSFER_MNEMONIC_MAX_LEN)
    mp_raise_ValueError("Invalid mnemonic length");
  manage_card_reset();
  return mp_obj_new_int(device_transfer_export_start(
      mnemonic.buf, (uint16_t)mnemonic.len, (uint32_t)listen, (uint32_t)turn));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(
    mod_trezorio_NFC_p2p_export_start_obj, 3, 3,
    mod_trezorio_NFC_p2p_export_start);

/// def p2p_export_poll() -> tuple[int, int, int, int, int]:
///     """Poll without waiting for RF. Returns (result, in, out, export_sw,
///     state). Result -1 means pending, 0 means success, 10 means cancelled;
///     other values are device_transfer_export_result_t.
///     """
STATIC mp_obj_t mod_trezorio_NFC_p2p_export_poll(void) {
  device_transfer_export_stats_t stats = {0};
  int result = device_transfer_export_poll(&stats);
  mp_obj_t values[] = {mp_obj_new_int(result),
                       mp_obj_new_int_from_uint(stats.frames_in),
                       mp_obj_new_int_from_uint(stats.frames_out),
                       mp_obj_new_int_from_uint(stats.export_sw),
                       mp_obj_new_int_from_uint(stats.last_state)};
  return mp_obj_new_tuple(5, values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_NFC_p2p_export_poll_obj,
                                 mod_trezorio_NFC_p2p_export_poll);

/// def p2p_export_close() -> None:
///     """Cancel and wipe the exporter session, then power NFC down.
///     Idempotent."""
STATIC mp_obj_t mod_trezorio_NFC_p2p_export_close(void) {
  device_transfer_export_close();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_NFC_p2p_export_close_obj,
                                 mod_trezorio_NFC_p2p_export_close);

STATIC const mp_rom_map_elem_t mod_trezorio_NFC_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_nfc)},
    {MP_ROM_QSTR(MP_QSTR_pwr_ctrl), MP_ROM_PTR(&mod_trezorio_NFC_pwr_ctrl_obj)},
    {MP_ROM_QSTR(MP_QSTR_poll_card),
     MP_ROM_PTR(&mod_trezorio_NFC_poll_card_obj)},
    {MP_ROM_QSTR(MP_QSTR_send_recv),
     MP_ROM_PTR(&mod_trezorio_NFC_send_recv_obj)},
    {MP_ROM_QSTR(MP_QSTR_close_secure_channel),
     MP_ROM_PTR(&mod_trezorio_NFC_close_secure_channel_obj)},
    {MP_ROM_QSTR(MP_QSTR_p2p_export_start),
     MP_ROM_PTR(&mod_trezorio_NFC_p2p_export_start_obj)},
    {MP_ROM_QSTR(MP_QSTR_p2p_export_poll),
     MP_ROM_PTR(&mod_trezorio_NFC_p2p_export_poll_obj)},
    {MP_ROM_QSTR(MP_QSTR_p2p_export_close),
     MP_ROM_PTR(&mod_trezorio_NFC_p2p_export_close_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mod_trezorio_NFC_globals,
                            mod_trezorio_NFC_globals_table);

STATIC const mp_obj_module_t mod_trezorio_NFC_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mod_trezorio_NFC_globals,
};
