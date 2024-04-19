#include STM32_HAL_H

#include "fpsensor_driver.h"
#include "fpsensor_platform.h"
#include "gt911.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  uint8_t irq_status[2];

  // fp sensor irq
  if (GPIO_Pin == GPIO_PIN_15) {
    fpsensor_read_irq_with_clear(irq_status, 2);
    if (irq_status[1] & 0x01) {
      fpsensor_state_set(true);
      HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    }
  }
  // touch panel irq
  else if (GPIO_Pin == GPIO_PIN_2) {
    gt911_read_location();
    HAL_NVIC_DisableIRQ(EXTI2_IRQn);
  }
}