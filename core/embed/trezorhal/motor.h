#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include STM32_HAL_H

// motor freq 240hz -> 4166.6666us/cycle -> round to 4160us
// #define MOTOR_TO_MAX_CURRENT_US 300
// #define MOTOR_HALF_CYCLE_US 2080

typedef enum {
  MOTOR_COAST = 0b00,
  MOTOR_FORWARD = 0b01,
  MOTOR_REVERSE = 0b10,
  MOTOR_BRAKE = 0b11,
} MOTOR_STATE;

typedef struct __attribute__((__packed__)) {
  MOTOR_STATE state;
  uint16_t duration_us;
} MOTOR_ACTION;

// function control
void motor_init(void);
void motor_deinit(void);
void motor_ctrl(MOTOR_ACTION* act);
bool motor_is_busy(void);
bool motor_play(MOTOR_ACTION* act_list, size_t act_list_len, bool by_cpu);
void motor_reset(void);

// debug functions
void motor_resonant_finder(uint16_t dur_f, uint16_t dur_r, uint16_t dur_b);

// builtin
void motor_set_builtin_play_method(bool by_cpu);

// builtin patterns
void motor_play_whisper(void);
void motor_play_light(void);
void motor_play_medium(void);
void motor_play_heavy(void);

// builtin sequences
void motor_play_success(void);
void motor_play_warning(void);
void motor_play_error(void);
void motor_play_slide(void);

#endif  // _MOTOR_H_
