#include STM32_HAL_H

#if !BOOT_ONLY
#include "fpsensor_driver.h"
#include "fpsensor_platform.h"
#include "gt911.h"
#endif
#include "spi_legacy.h"

void EXTI2_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2); }
void EXTI15_10_IRQHandler(void) {
  // fp sensor irq
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
  // spi cs irq
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
#if !BOOT_ONLY
  uint8_t irq_status[2];
  // fp sensor irq
  if (GPIO_Pin == GPIO_PIN_15) {
    fpsensor_read_irq_with_clear(irq_status, 2);
    if (irq_status[1] & 0x01) {
      fpsensor_state_set(true);
    }
  }
  // touch panel irq
  else if (GPIO_Pin == GPIO_PIN_2) {
    gt911_read_location();
  }
#endif
  // spi cs irq
  if (GPIO_Pin == GPIO_PIN_11) {
    spi_cs_irq_handler();
  }
}
