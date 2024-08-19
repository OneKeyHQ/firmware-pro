#include "adc.h"

#include STM32_HAL_H

typedef enum {
  ADC_DEVICE_INVALID_MIN = -1,
  ADC_DEVICE_HW_VER,
  ADC_DEVICE_INVALID_MAX,
} ADC_DEVICE_t;

// static ADC_TypeDef adc_controllers[ADC_DEVICE_INVALID_MAX];
// static uint16_t adc_controllers[ADC_DEVICE_INVALID_MAX];
// static GPIO_TypeDef adc_gpio_ports[ADC_DEVICE_INVALID_MAX];
// static uint16_t adc_gpio_pins[ADC_DEVICE_INVALID_MAX];

static bool adc_device_initialized[ADC_DEVICE_INVALID_MAX] = {0};
static ADC_HandleTypeDef adc_handles[ADC_DEVICE_INVALID_MAX] = {0};

static bool adc_init_device_hw_ver() {
  if (adc_device_initialized[ADC_DEVICE_HW_VER]) return false;

  // GPIO INIT

  // PA7     ------> ADC1_INP7

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // ADC INIT

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_CLKP;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
    return false;
  }
  __HAL_RCC_ADC12_CLK_ENABLE();
  __HAL_RCC_ADC12_FORCE_RESET();
  __HAL_RCC_ADC12_RELEASE_RESET();

  adc_handles[ADC_DEVICE_HW_VER].Instance = ADC1;
  adc_handles[ADC_DEVICE_HW_VER].Init.ScanConvMode = ADC_SCAN_DISABLE;
  adc_handles[ADC_DEVICE_HW_VER].Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  adc_handles[ADC_DEVICE_HW_VER].Init.LowPowerAutoWait = DISABLE;
  adc_handles[ADC_DEVICE_HW_VER].Init.ContinuousConvMode = DISABLE;
  adc_handles[ADC_DEVICE_HW_VER].Init.NbrOfConversion = 1;
  adc_handles[ADC_DEVICE_HW_VER].Init.DiscontinuousConvMode = DISABLE;
  adc_handles[ADC_DEVICE_HW_VER].Init.ExternalTrigConv = ADC_SOFTWARE_START;
  adc_handles[ADC_DEVICE_HW_VER].Init.ExternalTrigConvEdge =
      ADC_EXTERNALTRIGCONVEDGE_NONE;
  adc_handles[ADC_DEVICE_HW_VER].Init.ConversionDataManagement =
      ADC_CONVERSIONDATA_DR;
  adc_handles[ADC_DEVICE_HW_VER].Init.Overrun = ADC_OVR_DATA_PRESERVED;
  adc_handles[ADC_DEVICE_HW_VER].Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  adc_handles[ADC_DEVICE_HW_VER].Init.OversamplingMode = DISABLE;
  adc_handles[ADC_DEVICE_HW_VER].Init.Oversampling.Ratio = 1;
  // if ( HAL_ADC_Init(&adc_handles[ADC_DEVICE_HW_VER]) != HAL_OK )
  // {
  //     return false;
  // }
  adc_handles[ADC_DEVICE_HW_VER].Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
  adc_handles[ADC_DEVICE_HW_VER].Init.Resolution = ADC_RESOLUTION_16B;
  if (HAL_ADC_Init(&adc_handles[ADC_DEVICE_HW_VER]) != HAL_OK) {
    return false;
  }

  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&adc_handles[ADC_DEVICE_HW_VER],
                                       &multimode) != HAL_OK) {
    return false;
  }

  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&adc_handles[ADC_DEVICE_HW_VER], &sConfig) !=
      HAL_OK) {
    return false;
  }

  adc_device_initialized[ADC_DEVICE_HW_VER] = true;
  return true;
}

static bool adc_deinit_device_hw_ver() {
  if (!adc_device_initialized[ADC_DEVICE_HW_VER]) return false;

  __HAL_RCC_ADC12_FORCE_RESET();
  __HAL_RCC_ADC12_RELEASE_RESET();
  __HAL_RCC_ADC12_CLK_DISABLE();
  HAL_GPIO_DeInit(GPIOA, GPIO_PIN_7);

  adc_device_initialized[ADC_DEVICE_HW_VER] = false;

  return true;
}

bool adc_read_device_hw_ver(uint16_t* value) {
  if (!adc_device_initialized[ADC_DEVICE_HW_VER]) return false;

  /* Run the ADC calibration in single-ended mode */
  if (HAL_ADCEx_Calibration_Start(&adc_handles[ADC_DEVICE_HW_VER],
                                  ADC_CALIB_OFFSET_LINEARITY,
                                  ADC_SINGLE_ENDED) != HAL_OK) {
    /* Calibration Error */
    return false;
  }

  /*##-3- Start the conversion process*/
  if (HAL_ADC_Start(&adc_handles[ADC_DEVICE_HW_VER]) != HAL_OK) {
    /* Start Conversation Error */
    return false;
  }

  /*##-4- Wait for the end of conversion*/
  /*  For simplicity reasons, this example is just waiting till the end of the
      conversion, but application may perform other tasks while conversion
      operation is ongoing. */
  if (HAL_ADC_PollForConversion(&adc_handles[ADC_DEVICE_HW_VER], 10) !=
      HAL_OK) {
    /* End Of Conversion flag not set on time */
    return false;
  } else {
    /* ADC conversion completed */
    /*##-5- Get the converted value of regular channel*/
    *value = HAL_ADC_GetValue(&adc_handles[ADC_DEVICE_HW_VER]);
  }

  return true;
}

bool adc_init() {
  if (!adc_init_device_hw_ver()) return false;
  return true;
}

bool adc_deinit() {
  if (!adc_deinit_device_hw_ver()) return false;
  return true;
}
