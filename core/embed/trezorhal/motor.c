#include "motor.h"

#define ExecuteCheck_ADV(func_call, expected_result, on_false) \
  {                                                            \
    if ((func_call) != (expected_result)) {                    \
      on_false                                                 \
    }                                                          \
  }

#define ExecuteCheck_HAL_OK(func_call) \
  ExecuteCheck_ADV(func_call, HAL_OK, { return false; })

// example patterns
MOTOR_ACTION MAL_single[] = {
    {.state = MOTOR_FORWARD, .durnation_us = 2080},  // 0b10
    {.state = MOTOR_REVERSE, .durnation_us = 2080},  // 0b01
    {.state = MOTOR_BRAKE, .durnation_us = 2080},    // 0b11
    {.state = MOTOR_COAST, .durnation_us = 1500},    // 0b00
};
MOTOR_ACTION MAL_tick[] = {
    {.state = MOTOR_FORWARD, .durnation_us = 2080},  // 0b10
    {.state = MOTOR_REVERSE, .durnation_us = 2080},  // 0b01
    {.state = MOTOR_FORWARD, .durnation_us = 2080},  // 0b10
    {.state = MOTOR_REVERSE, .durnation_us = 2080},  // 0b01
    {.state = MOTOR_BRAKE, .durnation_us = 1000},    // 0b11
    {.state = MOTOR_COAST, .durnation_us = 1500},    // 0b00
};
MOTOR_ACTION MAL_tock[] = {
    {.state = MOTOR_FORWARD, .durnation_us = 2080},  // 0b10
    {.state = MOTOR_BRAKE, .durnation_us = 2080},    // 0b01
    {.state = MOTOR_COAST, .durnation_us = 1500},    // 0b00
};
MOTOR_ACTION MAL_relax[] = {
    {.state = MOTOR_COAST, .durnation_us = 1500},  // 0b00
};

// control status
static TIM_HandleTypeDef TIM7_Handle;
static bool motor_busy = false;
static MOTOR_ACTION* _act_list;
static size_t _act_list_len = 0;
static MOTOR_ACTION* _act_list_index = NULL;
static MOTOR_ACTION* _act_list_index_max = NULL;

static void motor_io_init(void) {
  GPIO_InitTypeDef gpio;
  __HAL_RCC_GPIOK_CLK_ENABLE();

  // PK2, PK3
  gpio.Pin = (GPIO_PIN_2 | GPIO_PIN_3);
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_PULLDOWN;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  gpio.Alternate = 0;
  HAL_GPIO_Init(GPIOK, &gpio);
}

inline void motor_ctrl(MOTOR_ACTION* act) {
  // as HAL_GPIO_WritePin only cares if PinState == GPIO_PIN_RESET
  // we don't have to filter non zero values
  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_2, ((*act).state & 0b0000001));
  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, ((*act).state & 0b0000010));
}

static bool motor_timer_init() {
  // 1000000hz = 1us period
  // 200 000 000hz clock
  // timer max 65535
  // upd_freq = TIM_CLK/(TIM_PSC+1)/(TIM_ARR + 1)
  // TIM_PSC = 0, TIM_ARR = 199

  // tim7
  __HAL_RCC_TIM7_CLK_ENABLE();

  TIM7_Handle.Instance = TIM7;
  TIM7_Handle.Init.Prescaler = 199;
  TIM7_Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TIM7_Handle.Init.Period = 0;
  ExecuteCheck_HAL_OK(HAL_TIM_Base_Init(&TIM7_Handle));
  ExecuteCheck_HAL_OK(HAL_TIM_OnePulse_Init(&TIM7_Handle, TIM_OPMODE_SINGLE));

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  ExecuteCheck_HAL_OK(
      HAL_TIMEx_MasterConfigSynchronization(&TIM7_Handle, &sMasterConfig));

  __HAL_TIM_CLEAR_IT(&TIM7_Handle, TIM_IT_UPDATE);
  __HAL_TIM_ENABLE_IT(&TIM7_Handle, TIM_IT_UPDATE);
  HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM7_IRQn);

  return true;
}
// static bool motor_timer_deinit()
// {
//     // tim7
//     __HAL_RCC_TIM7_CLK_DISABLE();
//     __HAL_TIM_DISABLE_IT(&TIM7_Handle, TIM_IT_UPDATE);
//     HAL_NVIC_DisableIRQ(TIM7_IRQn);
//     return true;
// }

void motor_cpu_play(MOTOR_ACTION* act_list, size_t act_list_len) {
  MOTOR_ACTION* act_list_index = act_list;
  while (act_list_index < (act_list + act_list_len)) {
    motor_ctrl(act_list_index);
    HAL_Delay((*act_list_index).durnation_us / 1000);
    act_list_index++;
  }
}

void TIM7_IRQHandler() {
  if (__HAL_TIM_GET_FLAG(&TIM7_Handle, TIM_FLAG_UPDATE) != RESET) {
    if (__HAL_TIM_GET_IT_SOURCE(&TIM7_Handle, TIM_IT_UPDATE) != RESET) {
      __HAL_TIM_CLEAR_IT(&TIM7_Handle, TIM_IT_UPDATE);

      if (_act_list_index < _act_list_index_max) {
        motor_ctrl(_act_list_index);
        _act_list_index++;
        __HAL_TIM_SET_AUTORELOAD(&TIM7_Handle,
                                 (*_act_list_index).durnation_us - 1);
        __HAL_TIM_ENABLE(&TIM7_Handle);
      } else {
        motor_reset();
        return;
      }
    }
  }
}

void motor_timer_play(MOTOR_ACTION* act_list, size_t act_list_len) {
  if (motor_busy) return;

  __HAL_TIM_DISABLE(&TIM7_Handle);
  _act_list = act_list;
  _act_list_len = act_list_len;
  _act_list_index =
      &act_list[0];  // explicitly point to first element in the array
  _act_list_index_max = act_list + act_list_len;

  __HAL_TIM_SET_AUTORELOAD(&TIM7_Handle, (*_act_list_index).durnation_us - 1);
  __HAL_TIM_ENABLE(&TIM7_Handle);
}

void motor_timer_reset(void) {
  _act_list_len = 0;
  _act_list = NULL;
  _act_list_index = NULL;
  _act_list_index_max = NULL;
}

void motor_init(void) {
  motor_io_init();
  motor_timer_init();
  motor_reset();
}

void motor_reset(void) {
  motor_ctrl(MAL_relax);
  motor_timer_reset();
  __HAL_TIM_DISABLE(&TIM7_Handle);
  motor_busy = false;
}

void motor_tick(void) {
  motor_timer_play(MAL_tick, sizeof(MAL_tick) / sizeof(MOTOR_ACTION));
}

void motor_tock(void) {
  motor_timer_play(MAL_tock, sizeof(MAL_tock) / sizeof(MOTOR_ACTION));
}

void motor_test(void) {
  motor_timer_play(MAL_single, sizeof(MAL_single) / sizeof(MOTOR_ACTION));
}

void motor_resonant_finder(uint16_t dur_f, uint16_t dur_r, uint16_t dur_b) {
  MOTOR_ACTION MAL_single_shot[] = {
      {.state = MOTOR_FORWARD, .durnation_us = dur_f},
      {.state = MOTOR_COAST, .durnation_us = dur_b},
      {.state = MOTOR_REVERSE, .durnation_us = dur_r},
      {.state = MOTOR_COAST, .durnation_us = dur_b},
  };
  motor_cpu_play(MAL_single_shot,
                 sizeof(MAL_single_shot) / sizeof(MOTOR_ACTION));
}