#include "thd89_boot.h"
#include "common.h"
#include "rand.h"
#include "thd89.h"

static uint8_t device_addr = THD89_MASTER_ADDRESS;

void thd89_boot_set_address(uint8_t addr) { device_addr = (addr << 1); }

static bool _se_get_state(uint8_t addr, uint8_t *state) {
  uint8_t cmd[5] = {0x80, 0xca, 0x00, 00, 0x00};
  uint16_t resp_len = 1;

  if (!thd89_transmit_ex(addr, cmd, sizeof(cmd), state, &resp_len)) {
    return false;
  }

  if ((resp_len != 0x01) ||
      ((state[0] != 0x00) && (state[0] != 0x55) && (state[0] != 0x33))) {
    return false;
  }
  return true;
}

char *se_get_version_ex(void) {
  uint8_t get_ver[5] = {0x00, 0xf7, 0x00, 00, 0x00};
  static char ver[16] = {0};
  uint16_t ver_len = sizeof(ver);

  if (!thd89_transmit_ex(device_addr, get_ver, sizeof(get_ver), (uint8_t *)ver,
                         &ver_len)) {
    return NULL;
  }

  return ver;
}

uint8_t se_get_state(void) {
  uint8_t state, boot_flag = 0;
  ensure(_se_get_state(THD89_1ST_ADDRESS, &state) ? sectrue : secfalse,
         "se1 get state failed");
  if (state == THD89_STATE_BOOT) {
    boot_flag |= THD89_1ST_IN_BOOT;
  }
  ensure(_se_get_state(THD89_2ND_ADDRESS, &state) ? sectrue : secfalse,
         "se2 get state failed");
  if (state == THD89_STATE_BOOT) {
    boot_flag |= THD89_2ND_IN_BOOT;
  }
  ensure(_se_get_state(THD89_3RD_ADDRESS, &state) ? sectrue : secfalse,
         "se3 get state failed");
  if (state == THD89_STATE_BOOT) {
    boot_flag |= THD89_3RD_IN_BOOT;
  }
  ensure(_se_get_state(THD89_4TH_ADDRESS, &state) ? sectrue : secfalse,
         "se4 get state failed");
  if (state == THD89_STATE_BOOT) {
    boot_flag |= THD89_4TH_IN_BOOT;
  }

  return boot_flag;
}

bool se_get_state_ex(uint8_t *state) {
  return _se_get_state(device_addr, state);
}

bool se_back_to_boot(void) {
  uint8_t cmd[5] = {0x80, 0xfc, 0x00, 0xff, 0x00};
  uint16_t resp_len = 0;
  if (!thd89_transmit_ex(device_addr, cmd, sizeof(cmd), NULL, &resp_len)) {
    return false;
  }
  return true;
}

bool se_active_app(void) {
  uint8_t cmd[5] = {0x80, 0xfc, 0x00, 0x04, 0x00};
  uint16_t resp_len = 0;
  if (!thd89_transmit_ex(device_addr, cmd, sizeof(cmd), NULL, &resp_len)) {
    return false;
  }
  return true;
}

bool se_update(uint8_t step, uint8_t *data, uint16_t data_len) {
  uint8_t cmd[1032];
  uint16_t cmd_len = 5, resp_len = 0;
  cmd[0] = 0x80;
  cmd[1] = 0xFC;
  cmd[2] = 0x00;
  cmd[3] = step;
  cmd[4] = 0x00;

  // send steps
  if (0x01 == step) {
    if (data_len != 1024) {
      return false;
    }
    cmd[5] = 0x04;
    cmd[6] = 0x00;
    memcpy(cmd + 7, data, data_len);
    cmd_len += 2 + data_len;

  } else if (0x02 == step) {
    if (data_len != 512) {
      return false;
    }
    cmd[5] = 0x02;
    cmd[6] = 0x00;
    memcpy(cmd + 7, data, 512);
    cmd_len += 2 + 512;
  }
  if (!thd89_transmit_ex(device_addr, cmd, cmd_len, NULL, &resp_len)) {
    return false;
  }
  return true;
}

bool se_back_to_boot_progress(void) {
  uint8_t state;
  if (!se_get_state_ex(&state)) {
    return false;
  }
  if (state == THD89_STATE_APP) {
    se_back_to_boot();
    hal_delay(1000);
    se_get_state_ex(&state);
  }
  if (state != THD89_STATE_BOOT) {
    return false;
  }
  return true;
}

bool se_verify_firmware(uint8_t *header, uint32_t header_len) {
  return se_update(1, header, header_len);
}

bool se_check_firmware(void) { return se_update(3, NULL, 0); }

bool se_update_firmware(uint8_t *data, uint32_t data_len,
                        void (*ui_callback)(int progress)) {
  uint32_t offset_len = 0;
  while (data_len) {
    uint32_t packet_len = data_len > 512 ? 512 : data_len;

    if (!se_update(2, data + offset_len, packet_len)) {
      return false;
    }
    data_len -= packet_len;
    offset_len += packet_len;
    if (ui_callback) {
      ui_callback(1000 * offset_len / (offset_len + data_len));
    }
  }

  return true;
}

bool se_active_app_progress(void) {
  if (!se_active_app()) {
    return false;
  }
  hal_delay(500);

  uint8_t state;
  if (!se_get_state_ex(&state)) {
    return false;
  }
  if (state != THD89_STATE_APP) {
    return false;
  }
  return true;
}
