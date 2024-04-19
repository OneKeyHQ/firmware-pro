#include STM32_HAL_H

#include "ble.h"
#include "camera.h"
#include "common.h"
#include "fpsensor_platform.h"
#include "mipi_lcd.h"
#include "sdram.h"
#include "systick.h"
#include "touch.h"
#include "usart.h"
#include "usb.h"

/* RTC handler declaration */
static RTC_HandleTypeDef RTCHandle;

#define RTC_ASYNCH_PREDIV 0x7F
#define RTC_SYNCH_PREDIV 0xF9 /* 32Khz/128 - 1 */

static bool rtc_inited = false;
static bool wakeup_by_rtc = false;

#define RTC_MAX_TIMEOUT 0xFFFF

void rtc_init(void) {
  if (rtc_inited) {
    return;
  }

  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable LSI clock */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    ensure(0, "HAL_RCC_OscConfig failed");
  }

  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

  /* Clocks structure declaration */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
    ensure(0, "HAL_RCCEx_PeriphCLKConfig failed");
  }

  __HAL_RCC_RTC_ENABLE();

  HAL_EXTI_D1_EventInputConfig(EXTI_LINE19, EXTI_MODE_IT, ENABLE);

  /* Configure RTC */
  RTCHandle.Instance = RTC;
  /* Configure RTC prescaler and RTC data registers as follow:
    - Hour Format = Format 24
    - Asynch Prediv = Value according to source clock
    - Synch Prediv = Value according to source clock
    - OutPut = Output Disable
    - OutPutPolarity = High Polarity
    - OutPutType = Open Drain */
  RTCHandle.Init.HourFormat = RTC_HOURFORMAT_24;
  RTCHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
  RTCHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
  RTCHandle.Init.OutPut = RTC_OUTPUT_WAKEUP;
  RTCHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  RTCHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

  if (HAL_RTC_Init(&RTCHandle) != HAL_OK) {
    ensure(0, "HAL_RTC_Init failed");
  }

  HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0x0, 0);
  HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
  rtc_inited = true;
}

void rtc_disable(void) { HAL_RTCEx_DeactivateWakeUpTimer(&RTCHandle); }

// period in seconds
void rtc_set_period(uint32_t period) {
  rtc_init();
  wakeup_by_rtc = false;
  HAL_RTCEx_DeactivateWakeUpTimer(&RTCHandle);
  if (period) {
    HAL_RTCEx_SetWakeUpTimer_IT(&RTCHandle, period,
                                RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
  }
}

void RTC_WKUP_IRQHandler(void) {
  HAL_RTCEx_WakeUpTimerIRQHandler(&RTCHandle);
  wakeup_by_rtc = true;
}

void enter_stop_mode(bool restart, uint32_t shutdown_seconds) {
  static uint32_t seconds = 0;
  static uint32_t rtc_period = 0;
  if (is_usb_connected()) {
    return;
  }
  if (restart) {
    seconds = shutdown_seconds;
  }
  camera_power_off();
  ble_disconnect();
  fpsensor_irq_disable();
  touch_enable_irq();
  usart_enable_stop_wup();
  lcd_refresh_suspend();
  sdram_set_self_refresh();
  while (seconds) {
    // previous rtc timeout
    if (wakeup_by_rtc) {
      seconds -= rtc_period;
      if (seconds == 0) {
        ble_power_off();
      }
      restart = true;
    }
    if (!restart) {
      restart = true;
    } else {
      rtc_period = seconds > RTC_MAX_TIMEOUT ? RTC_MAX_TIMEOUT : seconds;
      rtc_set_period(rtc_period);
    }
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    if (!wakeup_by_rtc) {
      break;
    }
    seconds -= rtc_period;
    if (seconds == 0) {
      ble_power_off();
    }
  }
  sdram_set_normal_mode();
  lcd_refresh_resume();
  touch_disable_irq();
  usart_disable_stop_wup();
}
