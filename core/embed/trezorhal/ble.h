#ifndef __BLE_H__
#define __BLE_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

extern uint8_t battery_cap;
extern uint8_t dev_pwr_sta;

#define BLE_NAME_LEN 16
#define CHARGE_BY_USB 0x01
#define CHARGE_BY_WIRELESS 0x02

// BLE send command
#define BLE_CMD_ADV 0x01
#define BLE_CMD_CON_STA 0x02
#define BLE_CMD_PAIR_TX 0x03
#define BLE_CMD_PAIR_STA 0x04
#define BLE_CMD_FM_VER 0x05
#define BLE_CMD_PROTO_VER 0x06
#define BLE_CMD_BOOT_VER 0x07
#define BLE_CMD_PLUG_STA 0x08
#define BLE_CMD_EQ 0x09
#define BLE_CMD_RPESS 0x0A
#define BLE_CMD_PWR 0x0B
#define BLE_CMD_FLASHLED_STATE 0x0C
#define BLE_CMD_BATTERY_INFO 0x0D
#define BLE_CMD_DEV_KEY 0x0E

#define BLE_KEY_RESP_SUCCESS 0x00
#define BLE_KEY_RESP_FAILED 0x01
#define BLE_KEY_RESP_PUBKEY 0x02
#define BLE_KEY_RESP_SIGN 0x03

// ST send command
#define BLE_BT 0x81
#define BLE_BT_ON 0x01
#define BLE_BT_OFF 0x02
#define BLE_BT_DISCON 0x03
#define BLE_BT_STA 0x04
#define BLE_PWR 0x82
#define BLE_PWR_SYS_OFF 0x01
#define BLE_PWR_EMMC_OFF 0x02
#define BLE_PWR_EMMC_ON 0x03
#define BLE_PWR_EQ 0x04
#define BLE_PWR_CHARGING 0x05
#define BLE_VER 0x83
#define BLE_VER_ADV 0x01
#define BLE_VER_FW 0x02
#define BLE_VER_PROTO 0x03
#define BLE_VER_BOOT 0x04
#define BLE_REBOOT 0x84
#define BLE_REBOOT_SYS 0x01

#define BLE_FLASHLED 0x85

#define BLE_BATTERY_INFO 0x86
#define BLE_BATTERY_INFO_VOLTAGE 0x01
#define BLE_BATTERY_INFO_CHARGING_CURRENT 0x02
#define BLE_BATTERY_INFO_DISCHARGING_CURRENT 0x03
#define BLE_BATTERY_INFO_INNER_TEMP 0x04

#define BLE_DEVICE_KEY 0x87
#define BLE_DEVICE_KEY_GET 0x01
#define BLE_DEVICE_KEY_LOCK 0x02
#define BLE_DEVICE_KEY_SIGN 0x03

bool ble_connect_state(void);
void ble_cmd_req(uint8_t cmd, uint8_t value);
void ble_cmd_req_ex(uint8_t cmd, const uint8_t *value, uint32_t value_len);
void ble_uart_poll(void);

#define ble_disconnect() ble_cmd_req(BLE_BT, BLE_BT_DISCON)
#define ble_power_off() ble_cmd_req(BLE_PWR, BLE_PWR_SYS_OFF)

#if !EMULATOR
bool ble_is_enable(void);
bool ble_name_state(void);
bool ble_ver_state(void);
bool ble_battery_state(void);
bool ble_charging_state(void);
uint32_t ble_power_button_state(void);
void ble_power_button_state_clear(void);
char *ble_get_name(void);
char *ble_get_ver(void);
bool ble_switch_state(void);
void ble_set_switch(bool flag);
bool ble_get_switch(void);
void ble_get_dev_info(void);
void ble_refresh_dev_info(void);
void ble_set_flashled(uint8_t value);
bool ble_get_version(char **ver);
bool ble_get_pubkey(uint8_t *pubkey);
bool ble_lock_pubkey(void);
bool ble_sign_msg(uint8_t *msg, uint32_t msg_len, uint8_t *sign);
uint8_t ble_get_charge_type(void);
bool ble_get_battery_voltage(uint16_t *voltage);
bool ble_get_battery_charging_current(uint16_t *current);
bool ble_get_battery_discharging_current(uint16_t *current);
bool ble_get_battery_inner_temp(uint16_t *temp);

#else
#define ble_name_state(...) false
#define ble_ver_state(...) false
#define ble_get_name(...) "OneKey814591011"
#define ble_get_ver(...) "1.0.1"
#define ble_switch_state(...) false
#define ble_set_switch(...)
#define ble_get_switch(...) false
#define change_ble_sta(...)
#endif

#endif
