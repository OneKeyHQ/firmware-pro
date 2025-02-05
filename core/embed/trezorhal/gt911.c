
#include STM32_HAL_H

#include "gt911.h"
#include "common.h"
#include "i2c.h"
#include "irq.h"

static I2C_HandleTypeDef *i2c_handle_touchpanel = NULL;
// static uint8_t gt911_data[256];

void gt911_io_init(void) {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStructure;

  /* Configure the GPIO Reset pin */
  GPIO_InitStructure.Pin = GPIO_PIN_1;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* Configure the GPIO Interrupt pin */
  GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
}

void gt911_reset(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.Pin = GPIO_PIN_2;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
  HAL_Delay(100);

  GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructure.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void gt911_read(uint16_t reg_addr, uint8_t *buf, uint16_t len) {
  if (HAL_I2C_Mem_Read(i2c_handle_touchpanel, GT911_ADDR, reg_addr, 2, buf, len,
                       1000) != HAL_OK) {
    ensure(secfalse, "gt911 read failed");
  }
}

void gt911_write(uint16_t reg_addr, uint8_t *buf, uint16_t len) {
  if (HAL_I2C_Mem_Write(i2c_handle_touchpanel, GT911_ADDR, reg_addr, 2, buf,
                        len, 1000) != HAL_OK) {
    ensure(secfalse, "gt911 write failed");
  }
}

// return one point data only
uint32_t gt911_read_location(void) {
  uint8_t point_data[10] = {0};
  uint8_t point_num;
  uint16_t x = 0, y = 0;
  static uint32_t xy = 0;

  static uint8_t last_point_num = 0;

  gt911_read(GTP_READ_COOR_ADDR, point_data, 10);
  if (point_data[0] == 0x00) {
    return xy;
  }

  if (point_data[0] == 0x80) {
    point_data[0] = 0;
    gt911_write(GTP_READ_COOR_ADDR, point_data, 1);
    last_point_num = 0;
    xy = 0;
    return 0;
  }
  point_num = point_data[0] & 0x0f;

  if (last_point_num == 0 && point_num == 1) {
    last_point_num = point_num;
  }

  if (point_num && last_point_num == 1) {
    x = point_data[2] | (point_data[3] << 8);
    y = point_data[4] | (point_data[5] << 8);
  }

  point_data[0] = 0;
  gt911_write(GTP_READ_COOR_ADDR, point_data, 1);

  xy = x << 16 | y;

  return xy;
}

void gt911_enter_sleep(void) {
  uint8_t data[1] = {0x05};
  gt911_write(GTP_REG_SLEEP, data, 1);
}

void gt911_enable_irq(void) {
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.Pin = GPIO_PIN_2;
  GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStructure.Pull = GPIO_NOPULL;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

  NVIC_SetPriority(EXTI2_IRQn, IRQ_PRI_GPIO);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}

void gt911_disable_irq(void) { HAL_NVIC_DisableIRQ(EXTI2_IRQn); }

void gt911_test(void) {
  while (1) {
    gt911_read_location();
  }
}

void gt911_init(void) {
  i2c_handle_touchpanel =
      &i2c_handles[i2c_find_channel_by_device(I2C_TOUCHPANEL)];
  gt911_io_init();
  gt911_reset();
  i2c_init_by_device(I2C_TOUCHPANEL);
}
