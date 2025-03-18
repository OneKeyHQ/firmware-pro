/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (c) SatoshiLabs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include STM32_HAL_H

#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "secbool.h"

#include "display.h"
#include "gt911.h"
#include "i2c.h"
#include "touch.h"

static bool touch_inited = false;
bool touch_is_inited() { return touch_inited; }

#if defined(GT911)

void touch_init(void) {
  gt911_init();
  touch_inited = true;
}

uint32_t touch_click(void) {
  uint32_t r = 0;
  r = touch_read();
  while (r) {
    r = touch_read();
    if ((r & TOUCH_END) == TOUCH_END) {
      break;
    }
  }

  return r;
}

uint32_t touch_read(void) {
  static uint32_t xy, last_xy = 0;
  static int touching = 0;

  xy = gt911_read_location();

  if (xy) {
    xy = touch_pack_xy(xy >> 16, xy & 0xffff);
  }

  if (xy != 0 && touching == 0) {
    touching = 1;
    last_xy = xy;
    return TOUCH_START | xy;
  }
  if (xy != 0 && touching == 1) {
    last_xy = xy;
    return TOUCH_MOVE | xy;
  }
  if (xy == 0 && touching == 1) {
    touching = 0;
    return TOUCH_END | last_xy;
  }
  return 0;
  ;
}

uint32_t touch_is_detected(void) {
  // check the interrupt line coming in from the CTPM.
  // the line goes low when a touch event is actively detected.
  // reference section 1.2 of "Application Note for FT6x06 CTPM".
  // we configure the touch controller to use "interrupt polling mode".
  return GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);
}

void touch_enter_sleep_mode(void) { gt911_enter_sleep(); }

void touch_enable_irq(void) { gt911_enable_irq(); }

void touch_disable_irq(void) { gt911_disable_irq(); }

#else

static I2C_HandleTypeDef *i2c_handle_touchpanel = NULL;

#define TOUCH_ADDRESS \
  (0x1AU << 1)  // the HAL requires the 7-bit address to be shifted by one bit
#define TOUCH_PACKET_SIZE 7U
#define EVENT_PRESS_NONE -1
#define EVENT_PRESS_DOWN 0x00U
#define EVENT_CONTACT 0x80U
#define EVENT_LIFT_UP 0x40U
#define EVENT_NO_EVENT 0xC0U
#define GESTURE_NO_GESTURE 0x00U
#define X_POS_MSB (touch_data[1])
#define X_POS_LSB ((touch_data[3] >> 4) & 0x0FU)
#define Y_POS_MSB (touch_data[2])
#define Y_POS_LSB (touch_data[3] & 0x0FU)

static void touch_gpio_init(void) {
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

  /* Activate XRES active low */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET); /* Deactivate XRES */
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
  HAL_Delay(10);                                      /* wait 300 ms */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET); /* Deactivate XRES */
  HAL_Delay(300);
}

void touch_init(void) {
  uint8_t id;
  i2c_handle_touchpanel =
      &i2c_handles[i2c_find_channel_by_device(I2C_TOUCHPANEL)];
  touch_gpio_init();
  i2c_init_by_device(I2C_TOUCHPANEL);

  if (HAL_I2C_Mem_Read(i2c_handle_touchpanel, TOUCH_ADDRESS, 0xa8, 1, &id, 1,
                       1000) == HAL_OK) {
  }
}

void touch_power_on(void) { return; }

void touch_power_off(void) { i2c_deinit_by_device(I2C_TOUCHPANEL); }

void touch_sensitivity(uint8_t value) {
  // set panel threshold (TH_GROUP) - default value is 0x12
  uint8_t touch_panel_threshold[] = {0x80, value};
  ensure(sectrue * (HAL_OK == HAL_I2C_Master_Transmit(
                                  i2c_handle_touchpanel, TOUCH_ADDRESS,
                                  touch_panel_threshold,
                                  sizeof(touch_panel_threshold), 10)),
         NULL);
}

uint32_t touch_is_detected(void) {
  // check the interrupt line coming in from the CTPM.
  // the line goes low when a touch event is actively detected.
  // reference section 1.2 of "Application Note for FT6x06 CTPM".
  // we configure the touch controller to use "interrupt polling mode".
  return GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);
}
#if 0
uint32_t touch_read(void) {
  static uint8_t touch_data[TOUCH_PACKET_SIZE];
  uint8_t previous_touch_data[TOUCH_PACKET_SIZE];
  uint32_t xy;
  if (!touch_is_detected()) {
    return 0;
  }
  display_printf("touch detected\n");
  if (HAL_I2C_Mem_Read(i2c_handle_touchpanel, TOUCH_ADDRESS, 0xD000, 2, touch_data, 7,
                       1000) != HAL_OK) {
    return 0;
  }
  if (touch_data[6] != 0xAB) {
    return 0;
  }
  display_printf("\n %02x %02x %02x %02x %02x %02x %02x \n", touch_data[0],
                 touch_data[1], touch_data[2], touch_data[3], touch_data[4],
                 touch_data[5], touch_data[6]);
  if (touch_data[0] == 0x06 && touch_data[5] == 0x01) {
    HAL_Delay(5);
    while (1) {
      if (HAL_I2C_Mem_Read(i2c_handle_touchpanel, TOUCH_ADDRESS, 0xD000, 2, touch_data, 7,
                           1000) != HAL_OK) {
        return 0;
      }
      display_printf("\n %02x %02x %02x %02x %02x %02x %02x \n", touch_data[0],
                     touch_data[1], touch_data[2], touch_data[3], touch_data[4],
                     touch_data[5], touch_data[6]);
      if (touch_data[6] != 0xAB) {
        return 0;
      }
      if (touch_data[5] > 1) {
        return 0;
      }
      if (touch_data[0] == 0x06) {
        memcpy(previous_touch_data, touch_data, TOUCH_PACKET_SIZE);
      }
      if (touch_data[0] == 0xAB) {
        break;
      }
    }
    memcpy(touch_data, previous_touch_data, TOUCH_PACKET_SIZE);
    // touch points
    if (touch_data[5] == 0x01) {
      xy = touch_pack_xy((X_POS_MSB << 4) | X_POS_LSB,
                         (Y_POS_MSB << 4) | Y_POS_LSB);
      // display_printf("x= %d,y=%d \n", (X_POS_MSB << 4) | X_POS_LSB,
      //                (Y_POS_MSB << 4) | Y_POS_LSB);
      return TOUCH_END | xy;
    }
  }

  return 0;
}
#else

uint32_t touch_num_detected(void) {
  uint8_t touch_data[TOUCH_PACKET_SIZE] = {0};
  if (HAL_I2C_Mem_Read(i2c_handle_touchpanel, TOUCH_ADDRESS, 0xD000, 2,
                       touch_data, 7, 1000) != HAL_OK) {
    return 0;
  }
  if (touch_data[6] != 0xAB) {
    return 0;
  }
  if (touch_data[0] == 0x06) {
    return touch_data[5] & 0x0F;
  }
  return 0;
}

uint32_t touch_read(void) {
  static uint8_t touch_data[TOUCH_PACKET_SIZE],
      previous_touch_data[TOUCH_PACKET_SIZE];
  static uint32_t xy;
  static int touching;

  int last_packet = 0;
  // if (!touch_is_detected()) {
  //   if (touching) {
  //     last_packet = 1;
  //   } else {
  //     return 0;
  //   }
  // }

  memset(touch_data, 0x00, sizeof(touch_data));
  if (HAL_I2C_Mem_Read(i2c_handle_touchpanel, TOUCH_ADDRESS, 0xD000, 2,
                       touch_data, 7, 50) != HAL_OK) {
    return 0;
  }

  if (touch_data[6] != 0xAB) {
    return 0;
  }

  if (touch_data[0] != 0x06) {
    if (touching) {
      last_packet = 1;
    } else {
      return 0;
    }
  }

  if (touch_data[0] == 0x06 && touch_data[5] == 0x01) {
    if (0 == memcmp(previous_touch_data, touch_data, TOUCH_PACKET_SIZE)) {
    } else {
      memcpy(previous_touch_data, touch_data, TOUCH_PACKET_SIZE);
    }
    xy = touch_pack_xy((X_POS_MSB << 4) | X_POS_LSB,
                       (Y_POS_MSB << 4) | Y_POS_LSB);
    if (touching == 0) {
      touching = 1;
      return TOUCH_START | xy;
    } else {
      return TOUCH_MOVE | xy;
    }
  }
  if (last_packet) {
    touching = 0;
    return TOUCH_END | xy;
  }
  return 0;
}
#endif
uint32_t touch_click(void) {
  uint32_t r = 0;
  r = touch_read();
  while (r) {
    r = touch_read();
    if ((r & TOUCH_END) == TOUCH_END) {
      break;
    }
  }

  return r;
}

uint32_t boot_touch_detect(uint32_t timeout) {
  uint32_t data, x_start, y_start, x_mov, y_mov;
  x_start = y_start = x_mov = y_mov = 0;

  for (int i = 0; i < timeout; i++) {
    data = touch_read();
    if (data != 0) {
      if (data & TOUCH_START) {
        x_start = x_mov = (data >> 12) & 0xFFF;
        y_start = y_mov = data & 0xFFF;
      }

      if (data & TOUCH_MOVE) {
        x_mov = (data >> 12) & 0xFFF;
        y_mov = data & 0xFFF;
      }

      if ((abs(x_start - x_mov) > 100) || (abs(y_start - y_mov) > 100)) {
        return 1;
      }
    }
    hal_delay(1);
  }

  return 0;
}

void touch_test(void) {
  int pos;
  int i = 0;
  while (1) {
    pos = touch_read();
    if (pos & TOUCH_START) {
      display_printf(" %d touch start x= %d,y=%d \n", i, (pos >> 12) & 0xfff,
                     pos & 0xfff);
    }
    if (pos & TOUCH_MOVE) {
      display_printf(" %d touch move x= %d,y=%d \n", i, (pos >> 12) & 0xfff,
                     pos & 0xfff);
    }
    if (pos & TOUCH_END) {
      display_printf(" %d touch end x= %d,y=%d \n", i, (pos >> 12) & 0xfff,
                     pos & 0xfff);
    }
    i++;
  }
}
#endif
