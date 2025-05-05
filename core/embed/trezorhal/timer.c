#include STM32_HAL_H

#include "lvgl.h"

#define LVGL_TIMER_PERIOD_MS 2
#define HAL_TICK_PERIOD_MS 1
#define PRESCALE_VALUE 1000

static TIM_HandleTypeDef TimHandle;
extern __IO uint32_t uwTick;

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

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  TIM_HandleTypeDef timer_handle;
  HAL_StatusTypeDef ret;
  __HAL_RCC_TIM3_CLK_ENABLE();

  timer_handle.Instance = TIM3;

  timer_handle.Init.Period =
      SystemCoreClock / PRESCALE_VALUE / (2 * 1000) * HAL_TICK_PERIOD_MS - 1;
  timer_handle.Init.Prescaler = PRESCALE_VALUE - 1;
  timer_handle.Init.ClockDivision = 0;
  timer_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
  timer_handle.Init.RepetitionCounter = 0;

  ret = HAL_TIM_Base_Init(&timer_handle);
  if (ret != HAL_OK) {
    return ret;
  }

  /*##-2- Configure the NVIC for TIMx ########################################*/
  /* Set the TIMx priority */
  HAL_NVIC_SetPriority(TIM3_IRQn, TickPriority, 0);

  /* Enable the TIMx global Interrupt */
  HAL_NVIC_EnableIRQ(TIM3_IRQn);

  // HAL_TIM_Base_Start_IT(&timer_handle);
  return HAL_OK;
}

void timer_init(void) { lvgl_timer_init(); }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM4) {
    // Call lv_tick_inc() every LVGL_TIMER_PERIOD_MS ms
    lv_tick_inc(LVGL_TIMER_PERIOD_MS);
  } else if (htim->Instance == TIM3) {
    uwTick++;
  }
}

void TIM4_IRQHandler(void) { HAL_TIM_IRQHandler(&TimHandle); }
void TIM3_IRQHandler(void) { HAL_TIM_IRQHandler(&TimHandle); }
