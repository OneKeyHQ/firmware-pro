#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <stdbool.h>
#include <stdint.h>

#include STM32_HAL_H

typedef enum __attribute__((__packed__)) {
  MOTOR_COAST = 0b00,
  MOTOR_FORWARD = 0b01,
  MOTOR_REVERSE = 0b10,
  MOTOR_BRAKE = 0b11,
} MOTOR_STATE;

typedef struct __attribute__((__packed__)) {
  MOTOR_STATE state;
  uint16_t durnation_us;
} MOTOR_ACTION;

void motor_init(void);

void motor_ctrl(MOTOR_ACTION* act);
void motor_timer_play(MOTOR_ACTION* act_list, size_t act_list_len);
void motor_cpu_play(MOTOR_ACTION* act_list, size_t act_list_len);

void motor_tick(void);
void motor_tock(void);
void motor_test(void);
void motor_resonant_finder(uint16_t dur_f, uint16_t dur_r, uint16_t dur_b);

#endif  // _MOTOR_H_
