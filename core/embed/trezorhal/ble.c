#include "ble.h"
#include "common.h"
#include "display.h"
#include "spi_legacy.h"
#include "usart.h"

uint8_t battery_cap = 0xFF;
uint8_t dev_pwr_sta = 0;

static usart_msg ble_usart_msg;
static bool get_ble_name = false;
static bool get_ble_ver = false;
static bool get_ble_proto_ver = false;
static bool get_ble_boot_ver = false;
static bool get_ble_battery = false;
static bool get_ble_charging = false;
static uint8_t ble_charging_type = 0;
static bool ble_connect = false;
static bool ble_switch = false;
static bool get_ble_switch = false;
static char ble_name[BLE_NAME_LEN + 1] = {0};
static char ble_ver[16] = {0};
static char ble_proto_ver[16 + 1] = {0};
static char ble_boot_ver[6] = {0};
static uint8_t dev_press_sta = 0;
static uint8_t dev_pwr = 0;
static int ble_request_state = -1;
static uint8_t ble_response_buf[64];

static uint8_t calXor(uint8_t *buf, uint32_t len) {
  uint8_t tmp = 0;
  uint32_t i;
  for (i = 0; i < len; i++) {
    tmp ^= buf[i];
  }
  return tmp;
}

static void ble_cmd_packet(uint8_t *value, uint8_t value_len) {
  uint8_t cmd[64] = {0};
  cmd[0] = 0x5a;
  cmd[1] = 0xa5;
  cmd[2] = ((value_len + 1) >> 8) & 0xff;
  cmd[3] = (value_len + 1) & 0xff;
  memcpy(cmd + 4, value, value_len);
  cmd[value_len + 4] = calXor(cmd, value_len + 4);
  ble_usart_send(cmd, value_len + 5);
}

void ble_cmd_req(uint8_t cmd, uint8_t value) {
  uint8_t buf[64] = {0};
  buf[0] = cmd;
  buf[1] = value;
  ble_cmd_packet(buf, 2);
  hal_delay(5);
}

void ble_cmd_req_ex(uint8_t cmd, const uint8_t *value, uint32_t value_len) {
  uint8_t buf[64] = {0};
  buf[0] = cmd;
  memcpy(buf + 1, value, value_len);
  ble_cmd_packet(buf, value_len + 1);
  hal_delay(5);
}

bool ble_get_version(char **ver) {
  // if (get_ble_ver) {
  //   *ver = ble_ver;
  //   return true;
  // }
  // ble_cmd_req(BLE_VER, BLE_VER_FW);
  // uint8_t counter = 0;
  // while (1) {
  //   ble_uart_poll();
  //   if (get_ble_ver) {
  //     break;
  //   }
  //   counter++;
  //   hal_delay(100);
  //   if (counter > 20) {
  //     return false;
  //   }
  //   ble_cmd_req(BLE_VER, BLE_VER_FW);
  // }
  ble_refresh_dev_info();
  *ver = ble_ver;
  return true;
}

bool ble_get_pubkey(uint8_t *pubkey) {
  uint8_t cmd[64] = {0};
  uint8_t counter = 0;
  cmd[0] = BLE_DEVICE_KEY;
  cmd[1] = BLE_DEVICE_KEY_GET;
  ble_cmd_packet(cmd, 2);
  ble_request_state = -1;
  while (1) {
    ble_uart_poll();
    if (ble_request_state != -1) {
      break;
    }
    counter++;
    hal_delay(1);
    // 2000ms
    if (counter > 2000) {
      return false;
    }
    // 100ms
    if (counter % 100 == 0) {
      ble_cmd_packet(cmd, 2);
    }
  }
  if (ble_request_state != 0) {
    return false;
  }
  memcpy(pubkey, ble_response_buf, 64);
  memset(ble_response_buf, 0x00, 64);
  return true;
}

bool ble_lock_pubkey(void) {
  uint8_t cmd[64] = {0};
  uint8_t counter = 0;
  cmd[0] = BLE_DEVICE_KEY;
  cmd[1] = BLE_DEVICE_KEY_LOCK;
  ble_cmd_packet(cmd, 2);
  ble_request_state = -1;
  while (1) {
    ble_uart_poll();
    if (ble_request_state != -1) {
      break;
    }
    counter++;
    hal_delay(1);
    // 2000ms
    if (counter > 2000) {
      return false;
    }
    // 100ms
    if (counter % 100 == 0) {
      ble_cmd_packet(cmd, 2);
    }
  }
  if (ble_request_state != 0) {
    return false;
  }
  return true;
}

bool ble_sign_msg(uint8_t *msg, uint32_t msg_len, uint8_t *sign) {
  uint8_t cmd[64] = {0};
  uint8_t counter = 0;
  cmd[0] = BLE_DEVICE_KEY;
  cmd[1] = BLE_DEVICE_KEY_SIGN;
  memcpy(cmd + 2, msg, msg_len);
  ble_cmd_packet(cmd, msg_len + 2);
  ble_request_state = -1;
  while (1) {
    ble_uart_poll();
    if (ble_request_state != -1) {
      break;
    }
    counter++;
    hal_delay(1);
    // 2000ms
    if (counter > 2000) {
      return false;
    }
    // 100ms
    if (counter % 100 == 0) {
      ble_cmd_packet(cmd, msg_len + 2);
    }
  }
  if (ble_request_state != 0) {
    return false;
  }
  memcpy(sign, ble_response_buf, 64);
  memset(ble_response_buf, 0x00, 64);
  return true;
}

bool ble_get_battery_info(uint8_t type, uint16_t *value) {
  uint8_t counter = 0;
  ble_cmd_req(BLE_BATTERY_INFO, type);
  ble_request_state = -1;
  while (1) {
    ble_uart_poll();
    if (ble_request_state != -1) {
      break;
    }
    counter++;
    hal_delay(1);
    // 100ms
    if (counter > 100) {
      return false;
    }
  }
  if (ble_request_state != 0) {
    return false;
  }

  *value = (ble_response_buf[0] << 8) + ble_response_buf[1];
  return true;
}

bool ble_get_battery_voltage(uint16_t *voltage) {
  return ble_get_battery_info(BLE_BATTERY_INFO_VOLTAGE, voltage);
}

bool ble_get_battery_charging_current(uint16_t *current) {
  return ble_get_battery_info(BLE_BATTERY_INFO_CHARGING_CURRENT, current);
}

bool ble_get_battery_discharging_current(uint16_t *current) {
  return ble_get_battery_info(BLE_BATTERY_INFO_DISCHARGING_CURRENT, current);
}

bool ble_get_battery_inner_temp(uint16_t *temp) {
  return ble_get_battery_info(BLE_BATTERY_INFO_INNER_TEMP, temp);
}

bool ble_connect_state(void) { return ble_connect; }

bool ble_name_state(void) { return get_ble_name; }

bool ble_ver_state(void) { return get_ble_ver; }

bool ble_battery_state(void) { return get_ble_battery; }

bool ble_switch_state(void) { return get_ble_switch; }

bool ble_charging_state(void) { return get_ble_charging; }

uint32_t ble_power_button_state(void) { return dev_press_sta; }

uint8_t ble_get_charge_type(void) { return ble_charging_type; }

// Since RELEASED event won't be reported
// we have to clear this locally cached status
void ble_power_button_state_clear(void) { dev_press_sta = 0; }

char *ble_get_name(void) { return ble_name; }

char *ble_get_ver(void) { return ble_ver; }

void ble_set_switch(bool flag) { ble_switch = flag; }

bool ble_get_switch(void) { return ble_switch; }

extern trans_fifo uart_fifo_in;

void ble_uart_poll(void) {
  uint32_t total_len, len;

  uint8_t passkey[7] = {0};
  uint8_t buf[128] = {0};

  total_len = fifo_lockdata_len(&uart_fifo_in);
  if (total_len < 5) {
    return;
  }

  fifo_read_peek(&uart_fifo_in, buf, 4);

  len = (buf[2] << 8) + buf[3];

  fifo_read_lock(&uart_fifo_in, buf, len + 3);

  ble_usart_msg.cmd = buf[4];
  ble_usart_msg.cmd_vale = buf + 5;

  switch (ble_usart_msg.cmd) {
    case BLE_CMD_ADV:
      strlcpy(ble_name, (char *)ble_usart_msg.cmd_vale, sizeof(ble_name));
      get_ble_name = true;
      break;
    case BLE_CMD_CON_STA:
      get_ble_switch = true;
      if (ble_usart_msg.cmd_vale[0] == 0x01) {
        ble_connect = true;
      } else if (ble_usart_msg.cmd_vale[0] == 0x02) {
        ble_connect = false;
      } else if (ble_usart_msg.cmd_vale[0] == 0x03) {
        ble_switch = true;
      } else if (ble_usart_msg.cmd_vale[0] == 0x04) {
        ble_switch = false;
      }
      break;
    case BLE_CMD_PAIR_TX:
      memcpy(passkey, ble_usart_msg.cmd_vale, 6);
      display_text_center(DISPLAY_RESX / 2, 346, "Bluetooth passkey:", -1,
                          FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
      display_text_center(DISPLAY_RESX / 2, 370, (char *)passkey, -1,
                          FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
      break;
    case BLE_CMD_PAIR_STA:
      if (ble_usart_msg.cmd_vale[0] == 0x01) {
        ble_connect = true;
      } else {
        ble_connect = false;
      }
      break;
    case BLE_CMD_FM_VER:
      memcpy(ble_ver, ble_usart_msg.cmd_vale, 5);
      get_ble_ver = true;
      break;
    case BLE_CMD_PROTO_VER:
      memcpy(ble_proto_ver, ble_usart_msg.cmd_vale, 16);
      get_ble_proto_ver = true;
      break;
    case BLE_CMD_BOOT_VER:
      memcpy(ble_boot_ver, ble_usart_msg.cmd_vale, 5);
      get_ble_boot_ver = true;
      break;
    case BLE_CMD_PLUG_STA:
      get_ble_charging = true;
      if (ble_usart_msg.cmd_vale[0] == 1 || ble_usart_msg.cmd_vale[0] == 3) {
        dev_pwr_sta = 1;
        if (ble_usart_msg.cmd_vale[1] == CHARGE_BY_USB ||
            ble_usart_msg.cmd_vale[1] == CHARGE_BY_WIRELESS) {
          ble_charging_type = ble_usart_msg.cmd_vale[1];
        }
      } else {
        dev_pwr_sta = 0;
        ble_charging_type = 0;
      }

      break;
    case BLE_CMD_EQ:
      get_ble_battery = true;
      battery_cap = ble_usart_msg.cmd_vale[0];
      break;
    case BLE_CMD_RPESS:
      dev_press_sta = ble_usart_msg.cmd_vale[0];
      break;
    case BLE_CMD_PWR:
      dev_pwr = ble_usart_msg.cmd_vale[0];
      break;
    case BLE_CMD_BATTERY_INFO:
      if (ble_usart_msg.cmd_vale[0] == BLE_BATTERY_INFO_VOLTAGE) {
        memcpy(ble_response_buf, ble_usart_msg.cmd_vale + 1, 2);
        ble_request_state = 0;
      } else if (ble_usart_msg.cmd_vale[0] ==
                 BLE_BATTERY_INFO_CHARGING_CURRENT) {
        memcpy(ble_response_buf, ble_usart_msg.cmd_vale + 1, 2);
        ble_request_state = 0;
      } else if (ble_usart_msg.cmd_vale[0] ==
                 BLE_BATTERY_INFO_DISCHARGING_CURRENT) {
        memcpy(ble_response_buf, ble_usart_msg.cmd_vale + 1, 2);
        ble_request_state = 0;
      } else if (ble_usart_msg.cmd_vale[0] == BLE_BATTERY_INFO_INNER_TEMP) {
        memcpy(ble_response_buf, ble_usart_msg.cmd_vale + 1, 2);
        ble_request_state = 0;
      }
      break;
    case BLE_CMD_DEV_KEY:
      if (ble_usart_msg.cmd_vale[0] == BLE_KEY_RESP_PUBKEY) {
        memcpy(ble_response_buf, ble_usart_msg.cmd_vale + 1, 64);
        ble_request_state = 0;
      } else if (ble_usart_msg.cmd_vale[0] == BLE_KEY_RESP_SIGN) {
        memcpy(ble_response_buf, ble_usart_msg.cmd_vale + 1, 64);
        ble_request_state = 0;
      } else if (ble_usart_msg.cmd_vale[0] == BLE_KEY_RESP_FAILED) {
        ble_request_state = 1;
      } else if (ble_usart_msg.cmd_vale[0] == BLE_KEY_RESP_SUCCESS) {
        ble_request_state = 0;
      }
      break;
    default:
      break;
  }
}

void ble_get_dev_info(void) {
  if (!ble_name_state()) {
    ble_cmd_req(BLE_VER, BLE_VER_ADV);
    hal_delay(5);
  }

  if (!ble_ver_state()) {
    ble_cmd_req(BLE_VER, BLE_VER_FW);
    hal_delay(5);
  }

  if (!ble_battery_state()) {
    ble_cmd_req(BLE_PWR, BLE_PWR_EQ);
    hal_delay(5);
  }

  if (!ble_charging_state()) {
    ble_cmd_req(BLE_PWR, BLE_PWR_CHARGING);
    hal_delay(5);
  }

  if (!ble_switch_state()) {
    ble_cmd_req(BLE_BT, BLE_BT_STA);
    hal_delay(5);
  }
}

void ble_set_flashled(uint8_t value) {
  uint8_t buf[64] = {0};
  buf[0] = BLE_FLASHLED;
  buf[1] = 0x01;
  buf[2] = value;
  ble_cmd_packet(buf, 3);
  hal_delay(10);
}

void ble_refresh_dev_info(void) {
  // get_ble_name = false;
  // while(false == get_ble_name)
  // {
  //   ble_cmd_req(BLE_VER, BLE_VER_ADV);
  //   hal_delay(5);
  //   ble_uart_poll();
  // }

  get_ble_ver = false;
  while (!get_ble_ver) {
    ble_cmd_req(BLE_VER, BLE_VER_FW);
    hal_delay(5);
    ble_uart_poll();
  }
}

// IO initialization is required before calling the function
void ble_reset(void) {
  BLE_RST_PIN_LOW();
  hal_delay(5);
  BLE_RST_PIN_HIGH();
  hal_delay(10);
}
