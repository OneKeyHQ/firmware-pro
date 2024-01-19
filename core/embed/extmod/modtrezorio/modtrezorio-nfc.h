
#include "embed/extmod/trezorobj.h"
#include "nfc.h"

/// package: trezorio.__init__

/// class NFC:
///     """
///     """
typedef struct _mp_obj_NFC_t {
  mp_obj_base_t base;
} mp_obj_NFC_t;

/// def __init__(
///     self,
/// ) -> None:
///     """
///     """
STATIC mp_obj_t mod_trezorio_NFC_make_new(const mp_obj_type_t* type,
                                          size_t n_args, size_t n_kw,
                                          const mp_obj_t* args) {
  mp_arg_check_num(n_args, n_kw, 0, 0, false);

  mp_obj_NFC_t* o = m_new_obj(mp_obj_NFC_t);
  o->base.type = type;

  return MP_OBJ_FROM_PTR(o);
}

/// def pwr_ctrl(self, on_off: bool) -> int:
///     """
///     Control NFC power.
///     """
STATIC mp_obj_t mod_trezorio_NFC_pwr_ctrl(mp_obj_t self, mp_obj_t on_off) {
  return mp_obj_new_int(nfc_pwr_ctl(mp_obj_is_true(on_off)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_trezorio_NFC_pwr_ctrl_obj,
                                 mod_trezorio_NFC_pwr_ctrl);

/// def wait_card(self, timeout_ms: int) -> int:
///     """
///     Wait for card with timeout.
///     """
STATIC mp_obj_t mod_trezorio_NFC_wait_card(mp_obj_t self, mp_obj_t timeout_ms) {
  return mp_obj_new_int(nfc_wait_card(mp_obj_get_int(timeout_ms)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_trezorio_NFC_wait_card_obj,
                                 mod_trezorio_NFC_wait_card);

/// def send_recv(self, send: bytearray) -> Tuple[int, bytearray]:
///     """
///     Send receive data through NFC.
///     """
STATIC mp_obj_t mod_trezorio_NFC_send_recv(mp_obj_t self, mp_obj_t send) {
  mp_buffer_info_t buf_capdu = {0};
  mp_get_buffer_raise(send, &buf_capdu, MP_BUFFER_READ);
  uint8_t buf_rapdu[PN532_InDataExchange_BUFF_SIZE];
  size_t len_rapdu = PN532_InDataExchange_BUFF_SIZE;

  NFC_STATUS status = nfc_send_recv(buf_capdu.buf, (uint16_t)buf_capdu.len,
                                    buf_rapdu, (uint16_t*)&len_rapdu);

  mp_obj_t result[2];
  result[0] = mp_obj_new_int(status);
  result[1] = mp_obj_new_bytes(buf_rapdu, len_rapdu);

  return mp_obj_new_tuple(2, result);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_trezorio_NFC_send_recv_obj,
                                 mod_trezorio_NFC_send_recv);

/// def send_recv_single_shot(self, send: bytearray, timeout_ms: int) -> Tuple[int, bytearray]:
///     """
///     Wait for card, then send receive data through NFC.
///     """
STATIC mp_obj_t mod_trezorio_NFC_send_recv_single_shot(mp_obj_t self,
                                                       mp_obj_t send,
                                                       mp_obj_t timeout_ms) {
  mp_buffer_info_t buf_capdu = {0};
  mp_get_buffer_raise(send, &buf_capdu, MP_BUFFER_READ);
  uint8_t buf_rapdu[PN532_InDataExchange_BUFF_SIZE];
  size_t len_rapdu = PN532_InDataExchange_BUFF_SIZE;

  NFC_STATUS status =
      nfc_send_recv_aio(buf_capdu.buf, (uint16_t)buf_capdu.len, buf_rapdu,
                        (uint16_t*)&len_rapdu, mp_obj_get_int(timeout_ms));

  mp_obj_t result[2];
  result[0] = mp_obj_new_int(status);
  result[1] = mp_obj_new_bytes(buf_rapdu, len_rapdu);

  return mp_obj_new_tuple(2, result);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mod_trezorio_NFC_send_recv_single_shot_obj,
                                 mod_trezorio_NFC_send_recv_single_shot);

// class attr
STATIC const mp_rom_map_elem_t mod_trezorio_NFC_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_pwr_ctrl), MP_ROM_PTR(&mod_trezorio_NFC_pwr_ctrl_obj)},
    {MP_ROM_QSTR(MP_QSTR_wait_card),
     MP_ROM_PTR(&mod_trezorio_NFC_wait_card_obj)},
    {MP_ROM_QSTR(MP_QSTR_send_recv),
     MP_ROM_PTR(&mod_trezorio_NFC_send_recv_obj)},
    {MP_ROM_QSTR(MP_QSTR_send_recv_single_shot),
     MP_ROM_PTR(&mod_trezorio_NFC_send_recv_single_shot_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mod_trezorio_NFC_locals_dict,
                            mod_trezorio_NFC_locals_dict_table);

STATIC const mp_obj_type_t mod_trezorio_NFC_type = {
    {&mp_type_type},
    .name = MP_QSTR_NFC,
    .make_new = mod_trezorio_NFC_make_new,
    .locals_dict = (void*)&mod_trezorio_NFC_locals_dict,
};
