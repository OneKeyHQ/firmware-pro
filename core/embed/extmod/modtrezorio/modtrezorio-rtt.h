#include "SEGGER_RTT.h"
#include "embed/extmod/trezorobj.h"
#include "py/runtime.h"
#include "py/stream.h"

#define RTT_CHANNEL 1

STATIC mp_uint_t rtt_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size,
                           int *errcode) {
  (void)self_in;  // unused
  size_t written = SEGGER_RTT_Write(RTT_CHANNEL, buf_in, size);
  if (written != size) {
    *errcode = MP_EIO;  // Or another appropriate error
  }
  return written;
}

STATIC mp_uint_t rtt_read(mp_obj_t self_in, void *buf_in, mp_uint_t size,
                          int *errcode) {
  (void)self_in;  // unused
  size_t read = SEGGER_RTT_Read(RTT_CHANNEL, buf_in, size);
  if (read == 0) {
    *errcode = MP_EAGAIN;  // Or handle as needed
  }
  return read;
}

STATIC mp_uint_t rtt_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg,
                           int *errcode) {
  (void)self_in;  // unused
  switch (request) {
    case MP_STREAM_POLL: {
      mp_uint_t ret = 0;
      if (SEGGER_RTT_HasData(RTT_CHANNEL)) {
        ret |= MP_STREAM_POLL_RD;
      }
      // Add write poll if needed.
      return ret;
    }
    default:
      *errcode = MP_EINVAL;
      return 0;
  }
}

STATIC const mp_stream_p_t rtt_stream_p = {
    .read = rtt_read,
    .write = rtt_write,
    .ioctl = rtt_ioctl,
};

typedef struct _rtt_obj_t {
  mp_obj_base_t base;
} rtt_obj_t;

STATIC const mp_obj_type_t rtt_type = {
    {&mp_type_type},
    .name = MP_QSTR_RTT,
    .protocol = &rtt_stream_p,
};

STATIC const mp_rom_map_elem_t rtt_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_rtt)},
    {MP_ROM_QSTR(MP_QSTR_RTT), MP_ROM_PTR(&rtt_type)},
};

STATIC MP_DEFINE_CONST_DICT(rtt_module_globals, rtt_module_globals_table);

const mp_obj_module_t rtt_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&rtt_module_globals,
};
