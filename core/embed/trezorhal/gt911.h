#ifndef _GT911_H_
#define _GT911_H_

#include <stdbool.h>

#include "util_macros.h"

#define GT911

#define GT911_ADDR (0x5D << 1)
#define GTP_REG_VERSION 0x8140
#define GTP_REG_GESTURE 0x814B
#define GTP_READ_COOR_ADDR 0x814E
#define GTP_READ_POINT_ADDR 0x8158
#define GTP_REG_COMMAND 0x8040
#define GTP_REG_COMMAND_CHECK 0x8046
#define GTP_REG_MODSWITCH1 0x804D

#define GTP_REG_CONFIG_DATA 0x8047
#define GTP_REG_CONFIG_SHAKECOUNT 0x804F
#define GTP_REG_CONFIG_FILTER 0x8050
#define GTP_REG_CONFIG_LARGE_TOUCH 0x8051
#define GTP_REG_CONFIG_NOISE_REDUCTION 0x8052
#define GTP_REG_CONFIG_TOUCH_LEVEL 0x8053
#define GTP_REG_CONFIG_LEAVE_LEVEL 0x8054

// clang-format off

typedef struct __attribute__((packed)) {
    uint8_t config_version;          // 0x8047: Configuration version number, updated based on version or content, numbered from 'A' to 'Z', reset value is 'A'
    uint8_t x_output_max_low;        // 0x8048: X axis resolution low byte
    uint8_t x_output_max_high;       // 0x8049: X axis resolution high byte
    uint8_t y_output_max_low;        // 0x804A: Y axis resolution low byte
    uint8_t y_output_max_high;       // 0x804B: Y axis resolution high byte
    uint8_t touch_number;            // 0x804C: Number of touch points (1-5)
    uint8_t module_switch1;          // 0x804D: Module switch1, controls driver/sensor inversion, etc.
    uint8_t module_switch2;          // 0x804E: Module switch2, enables proximity/hotkey, etc.
    uint8_t shake_count;             // 0x804F: Debounce frequency for touch press/release, coefficient is 4
    uint8_t first_filter;            // 0x8050: Filter threshold for raw coordinates
    uint8_t large_touch;             // 0x8051: Number of large touch points
    uint8_t noise_reduction;         // 0x8052: Noise reduction value (0-15), coefficient is 1
    uint8_t screen_touch_level;      // 0x8053: Touch detection threshold
    uint8_t screen_leave_level;      // 0x8054: Touch release threshold
    uint8_t low_power_control;       // 0x8055: Interval for entering low power mode (0-15s)
    uint8_t refresh_rate;            // 0x8056: Coordinate reporting rate, includes gesture wake-up pulse width
    uint8_t x_threshold;             // 0x8057: X coordinate output threshold (0-255), 0 means continuous output
    uint8_t y_threshold;             // 0x8058: Y coordinate output threshold (0-255), 0 means continuous output
    uint8_t x_speed_limit;           // 0x8059: X speed limit, includes reserved bits
    uint8_t y_speed_limit;           // 0x805A: Y speed limit, includes reserved bits
    uint8_t space_top_bottom;        // 0x805B: Top/bottom border space, coefficient is 32
    uint8_t space_left_right;        // 0x805C: Left/right border space, coefficient is 32
    uint8_t mini_filter;             // 0x805D: Mini filter used in tracing lines, if configured value is 0, automatically sets to 4
    uint8_t stretch_r0;              // 0x805E: Stretch space 1 coefficient
    uint8_t stretch_r1;              // 0x805F: Stretch space 2 coefficient
    uint8_t stretch_r2;              // 0x8060: Stretch space 3 coefficient
    uint8_t stretch_rm;              // 0x8061: Base for multiple stretch spaces
    uint8_t drv_group_a_num;         // 0x8062: Number of driver group A, includes reserved bits
    uint8_t drv_group_b_num;         // 0x8063: Number of driver group B, dual frequency bit
    uint8_t sensor_num;              // 0x8064: Number of sensor groups A/B
    uint8_t freq_a_factor;           // 0x8065: Driver group A frequency clock multiplication factor
    uint8_t freq_b_factor;           // 0x8066: Driver group B frequency clock multiplication factor
    uint8_t pannel_bitfreq_l;        // 0x8067: Driver groups A/B base frequency low byte
    uint8_t pannel_bitfreq_h;        // 0x8068: Driver groups A/B base frequency high byte
    uint8_t pannel_sensor_time_l;    // 0x8069: Driver signal output interval low byte
    uint8_t pannel_sensor_time_h;    // 0x806A: Driver signal output interval high byte
    uint8_t pannel_drv_output_gain;  // 0x806B: Driver output gain, 0-7
    uint8_t pannel_rx_gain;          // 0x806C: Receiver gain setting
    uint8_t pannel_dump_shift;       // 0x806D: Gesture mode raw data amplification factor
    uint8_t drv_frame_control;       // 0x806E: Subframe driver number and repeat number
    uint8_t charging_level_up;       // 0x806F: Charging mode level adjustment
    uint8_t gesture_module_switch3;  // 0x8070: Gesture frequency hopping disable, etc.
    uint8_t gesture_dis;             // 0x8071: Sliding wake-up effective distance
    uint8_t gesture_long_press_time; // 0x8072: Long touch stops gesture recognition time, unit 100ms
    uint8_t x_y_slope_adjust;        // 0x8073: X/Y direction slope adjustment
    uint8_t gesture_control;         // 0x8074: Gesture control, double-click invalid time
    uint8_t gesture_switch1;         // 0x8075: Gesture switch1, sliding direction
    uint8_t gesture_switch2;         // 0x8076: Gesture switch2, bottom validity
    uint8_t gesture_refresh_rate;    // 0x8077: Gesture mode reporting rate, 5+ms
    uint8_t gesture_touch_level;     // 0x8078: Gesture mode touch threshold
    uint8_t newgreen_wakeup_level;   // 0x8079: NewGreen wake-up threshold
    uint8_t freq_hopping_start;      // 0x807A: Frequency hopping start frequency
    uint8_t freq_hopping_end;        // 0x807B: Frequency hopping end frequency
    uint8_t noise_detect_times;      // 0x807C: Noise detection times
    uint8_t hopping_flag;            // 0x807D: Frequency hopping flag, enable, etc.
    uint8_t hopping_threshold;       // 0x807E: Frequency hopping threshold, minimum 5
    uint8_t noise_threshold;         // 0x807F: Noise discrimination threshold
    uint8_t noise_min_threshold;     // 0x8080: ESD quick reduction threshold
    uint8_t reserved_8081;           // 0x8081: Reserved (NC)
    uint8_t hopping_sensor_group;    // 0x8082: Frequency hopping sensor group, recommended 4 parts
    uint8_t hopping_seg1_normalize;  // 0x8083: Seg1 normalization coefficient
    uint8_t hopping_seg1_factor;     // 0x8084: Seg1 center point factor
    uint8_t main_clock_adjust;       // 0x8085: Main clock frequency fine-tuning
    uint8_t hopping_seg2_normalize;  // 0x8086: Seg2 normalization coefficient
    uint8_t hopping_seg2_factor;     // 0x8087: Seg2 center point factor
    uint8_t reserved_8088;           // 0x8088: Reserved (NC)
    uint8_t hopping_seg3_normalize;  // 0x8089: Seg3 normalization coefficient
    uint8_t hopping_seg3_factor;     // 0x808A: Seg3 center point factor
    uint8_t reserved_808B;           // 0x808B: Reserved (NC)
    uint8_t hopping_seg4_normalize;  // 0x808C: Seg4 normalization coefficient
    uint8_t hopping_seg4_factor;     // 0x808D: Seg4 center point factor
    uint8_t reserved_808E;           // 0x808E: Reserved (NC)
    uint8_t hopping_seg5_normalize;  // 0x808F: Seg5 normalization coefficient
    uint8_t hopping_seg5_factor;     // 0x8090: Seg5 center point factor
    uint8_t reserved_8091;           // 0x8091: Reserved (NC)
    uint8_t hopping_seg6_normalize;  // 0x8092: Seg6 normalization coefficient
    uint8_t key_1;                   // 0x8093: Key1 address
    uint8_t key_2;                   // 0x8094: Key2 address
    uint8_t key_3;                   // 0x8095: Key3 address
    uint8_t key_4;                   // 0x8096: Key4 address
    uint8_t key_area;                // 0x8097: Key active area
    uint8_t key_touch_level;         // 0x8098: Key touch threshold
    uint8_t key_leave_level;         // 0x8099: Key release threshold
    uint8_t key_sens_key1_key2;      // 0x809A: Key1 and Key2 sensitivity
    uint8_t key_sens_key3_key4;      // 0x809B: Key3 and Key4 sensitivity
    uint8_t key_restrain;            // 0x809C: Key restriction interval
    uint8_t key_restrain_time;       // 0x809D: Key restriction time
    uint8_t gesture_large_touch;     // 0x809E: Gesture mode large area touch
    uint8_t reserved_809F;           // 0x809F: Reserved (NC)
    uint8_t reserved_80A0;           // 0x80A0: Reserved (NC)
    uint8_t hotknot_noise_map;       // 0x80A1: HotKnot noise map
    uint8_t reserved_80A2_to_80FE[0x80FE - 0x80A1];
    uint8_t check_sum;           // 0x80FF: Check sum
    uint8_t config_refresh;      // 0x8100: Configuration refresh
} GT911_Config_t;

// clang-format on
#define GTP_ENUM_ITEM(b, c) JOIN_EXPR(GTP, b, c)
// regex -> (GTP_ENUM_ITEM\((.*), (.*)\).*,).*
// replace -> $1 // GTP_$2_$3

typedef enum {
  GTP_ENUM_ITEM(REG_RT, CMD) = 0x8040,    // GTP_REG_RT_CMD
  GTP_ENUM_ITEM(REG_RT, CMDCK) = 0x8046,  // GTP_REG_RT_CMDCK
} GTP_REG_REALTIME_t;

typedef enum {
  GTP_ENUM_ITEM(REG_CONF, VER) = 0x8047,           // GTP_REG_CONF_VER
  GTP_ENUM_ITEM(REG_CONF, MODSWITCH1) = 0x804D,    // GTP_REG_CONF_MODSWITCH1
  GTP_ENUM_ITEM(REG_CONF, REFRESH_RATE) = 0x8056,  // GTP_REG_CONF_REFRESH_RATE

  GTP_ENUM_ITEM(REG_CONF, MODSWITCH3) = 0x8070,  // GTP_REG_CONF_MODSWITCH3
  GTP_ENUM_ITEM(REG_CONF, GEST_DIST) = 0x8071,   // GTP_REG_CONF_GEST_DIST
  GTP_ENUM_ITEM(REG_CONF, GEST_LG_TOUCH) =
      0x809E,  // GTP_REG_CONF_GEST_LG_TOUCH

  GTP_ENUM_ITEM(REG_CONF, CHKSUM) = 0x80FF,  // GTP_REG_CONF_CHKSUM
  GTP_ENUM_ITEM(REG_CONF, CHANGE_NOTIFY) =
      0x8100,  // GTP_REG_CONF_CHANGE_NOTIFY
} GTP_REG_CONFIG_t;

typedef enum {
  GTP_ENUM_ITEM(REG_CORD, PID_B0) = 0x8140,  // GTP_REG_CORD_PID_B0
  GTP_ENUM_ITEM(REG_CORD, PID_B2) = 0x8141,  // GTP_REG_CORD_PID_B2
  GTP_ENUM_ITEM(REG_CORD, PID_B3) = 0x8142,  // GTP_REG_CORD_PID_B3
  GTP_ENUM_ITEM(REG_CORD, PID_B4) = 0x8143,  // GTP_REG_CORD_PID_B4
  GTP_ENUM_ITEM(REG_CORD, FW_L) = 0x8144,    // GTP_REG_CORD_FW_L
  GTP_ENUM_ITEM(REG_CORD, FW_H) = 0x8145,    // GTP_REG_CORD_FW_H
  GTP_ENUM_ITEM(REG_CORD, VID) = 0x814A,     // GTP_REG_CORD_VID
  GTP_ENUM_ITEM(REG_CORD, STATUS) = 0x814E,  // GTP_REG_CORD_STATUS
} GTP_REG_CORDINATE_t;

typedef enum {
  GTP_ENUM_ITEM(REG_GEST, CTRL) = 0x8074,  // GTP_REG_GEST_CTRL
  GTP_ENUM_ITEM(REG_GEST, SW1) = 0x8075,   // GTP_REG_GEST_SW1
  GTP_ENUM_ITEM(REG_GEST, SW2) = 0x8076,   // GTP_REG_GEST_SW2

  GTP_ENUM_ITEM(REG_GEST, GID_B0) = 0x8140,  // GTP_REG_GEST_GID_B0
  GTP_ENUM_ITEM(REG_GEST, GID_B2) = 0x8141,  // GTP_REG_GEST_GID_B2
  GTP_ENUM_ITEM(REG_GEST, GID_B3) = 0x8142,  // GTP_REG_GEST_GID_B3
  GTP_ENUM_ITEM(REG_GEST, GID_B4) = 0x8143,  // GTP_REG_GEST_GID_B4
  GTP_ENUM_ITEM(REG_GEST, FW_L) = 0x8144,    // GTP_REG_GEST_FW_L
  GTP_ENUM_ITEM(REG_GEST, FW_H) = 0x8145,    // GTP_REG_GEST_FW_H

  GTP_ENUM_ITEM(REG_GEST, TYPE) = 0x814B,         // GTP_REG_GEST_TYPE
  GTP_ENUM_ITEM(REG_GEST, POINT_COUNT) = 0x814C,  // GTP_REG_GEST_POINT_COUNT
} GTP_REG_GESTURE_t;

typedef struct __attribute__((packed)) {
  // begin at GTP_REG_CONF_GEST_DIST

  uint8_t slide_distance;
  uint8_t long_press_time;
  uint8_t xy_slope_adj;
  uint8_t control;
  uint8_t sw1;
  uint8_t sw2;
  uint8_t refresh_rate;
  uint8_t threshold;

} GTP_CONF_GESTURE_t;

typedef struct __attribute__((packed)) {
  // begin at GTP_REG_GEST_TYPE

  uint8_t g_type;
  uint8_t g_points_count;
  uint16_t g_cord_start_x;
  uint16_t g_cord_start_y;
  uint16_t g_cord_end_x;
  uint16_t g_cord_end_y;

} GTP_GESTURE_STATUS_t;

typedef struct __attribute__((packed)) {
  // begin at GTP_REG_GEST_GID_B0

  char g_id[4];
  uint16_t fw_ver;
  uint16_t res_x;
  uint16_t res_y;

  uint8_t reserved0;

  GTP_GESTURE_STATUS_t g_status;

} GTP_GESTURE_INFO_t;

void gt911_init(void);
uint32_t gt911_read_location(void);
void gt911_enter_sleep(void);
void gt911_enable_irq(void);
void gt911_disable_irq(void);
bool gt911_wait_gesture(GTP_GESTURE_INFO_t* gi, GTP_GESTURE_STATUS_t* gs);
void gt911_test(void);
void gt911_test_gesture(void);

#endif
