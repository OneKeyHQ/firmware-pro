#include STM32_HAL_H

#include "lvgl.h"

#define LVGL_TIMER_PERIOD_MS 2

#define PRESCALE_VALUE 1000

static TIM_HandleTypeDef TimHandle;
static void lvgl_timer_init(void) {
  __HAL_RCC_TIM4_CLK_ENABLE();

  TimHandle.Instance = TIM4;

  TimHandle.Init.Period =
      SystemCoreClock / PRESCALE_VALUE / (2 * 1000) * LVGL_TIMER_PERIOD_MS - 1;
  TimHandle.Init.Prescaler = PRESCALE_VALUE - 1;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TimHandle.Init.RepetitionCounter = 0;

  if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK) {
  }

  /*##-2- Configure the NVIC for TIMx ########################################*/
  /* Set the TIMx priority */
  HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);

  /* Enable the TIMx global Interrupt */
  HAL_NVIC_EnableIRQ(TIM4_IRQn);

  HAL_TIM_Base_Start_IT(&TimHandle);
}

void timer_init(void) { lvgl_timer_init(); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM4) {
    // Call lv_tick_inc() every LVGL_TIMER_PERIOD_MS ms
    lv_tick_inc(LVGL_TIMER_PERIOD_MS);
  }
}

void TIM4_IRQHandler(void) { HAL_TIM_IRQHandler(&TimHandle); }
