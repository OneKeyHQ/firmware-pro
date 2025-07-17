/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (c) SatoshiLabs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREZORHAL_TOUCH_H
#define TREZORHAL_TOUCH_H

#include <stdint.h>

#if PRODUCTION_MODEL == 'H'
#define TS_I2C_PORT GPIOB
#define TS_I2C_SCL_PIN GPIO_PIN_10
#define TS_I2C_SDA_PIN GPIO_PIN_11

#define TS_RESET_PORT GPIOC
#define TS_RESET_PIN GPIO_PIN_1

#define TS_INIT_PORT GPIOC
#define TS_INIT_PIN GPIO_PIN_0

#else
#define TS_I2C_PORT GPIOB
#define TS_I2C_SCL_PIN GPIO_PIN_6
#define TS_I2C_SDA_PIN GPIO_PIN_7

#define TS_RESET_PORT GPIOC
#define TS_RESET_PIN GPIO_PIN_5

#define TS_INIT_PORT GPIOC
#define TS_INIT_PIN GPIO_PIN_4
#endif

#define TOUCH_START (1U << 24)
#define TOUCH_MOVE (1U << 25)
#define TOUCH_END (1U << 26)

typedef enum {
  GESTURE_NONE = 0,
  GESTURE_CLICK,
  GESTURE_SWIPE_UP,
  GESTURE_SWIPE_DOWN,
  GESTURE_SWIPE_LEFT,
  GESTURE_SWIPE_RIGHT
} touch_gesture_t;

typedef enum {
  SWIPE_NONE = 0,
  SWIPE_UP,
  SWIPE_DOWN,
  SWIPE_LEFT,
  SWIPE_RIGHT
} swipe_direction_t;

typedef struct {
  bool touching;
  uint16_t start_x;
  uint16_t start_y;
  uint16_t last_x;
  uint16_t last_y;
  uint32_t start_time;
  uint32_t last_time;
  bool moved;
  float smoothed_dx;
  float smoothed_dy;
  swipe_direction_t swipe_dir;
  int scroll_delta;
} touch_state_t;

typedef struct {
  touch_gesture_t gesture;
  uint16_t start_x;
  uint16_t start_y;
  uint16_t end_x;
  uint16_t end_y;
  uint32_t end_pos;
  uint16_t distance;
  uint32_t duration;
  uint16_t velocity;
  int scroll_delta;
} gesture_result_t;

#define CLICK_THRESHOLD_PIXELS 10
#define CLICK_THRESHOLD_TIME_MS 300
#define SWIPE_THRESHOLD_PIXELS 5
#define SMOOTH_ALPHA 0.5f

void touch_test(void);

bool touch_is_inited();
void touch_init(void);
// void touch_power_on(void);
void touch_power_off(void);
void touch_sensitivity(uint8_t value);
uint32_t touch_num_detected(void);
uint32_t touch_read(void);
uint32_t touch_click(void);
uint32_t touch_is_detected(void);
uint32_t boot_touch_detect(uint32_t timeout);
void touch_enter_sleep_mode(void);
void touch_enable_irq(void);
void touch_disable_irq(void);
gesture_result_t touch_gesture_detect(void);
static inline uint16_t touch_unpack_x(uint32_t evt) {
  return (evt >> 12) & 0xFFF;
}
static inline uint16_t touch_unpack_y(uint32_t evt) {
  return (evt >> 0) & 0xFFF;
}
static inline uint32_t touch_pack_xy(uint16_t x, uint16_t y) {
  return ((x & 0xFFF) << 12) | (y & 0xFFF);
}

#endif
