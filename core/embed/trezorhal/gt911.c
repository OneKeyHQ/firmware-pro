
#include STM32_HAL_H

#include "gt911.h"
#include "common.h"
#include "i2c.h"
#include "irq.h"

#include "debug_utils.h"
#include "display.h"
#include "util_macros.h"

#define GT911_ECA_R_FALSE(expr, expected_result) \
  ExecuteCheck_ADV(expr, expected_result, { return false; })

static I2C_HandleTypeDef* i2c_handle_touchpanel = NULL;
static uint8_t gt911_buffer[16];

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

bool gt911_read(uint16_t reg_addr, uint8_t* buf, uint16_t len) {
  GT911_ECA_R_FALSE(HAL_I2C_Mem_Read(i2c_handle_touchpanel, GT911_ADDR,
                                     reg_addr, 2, buf, len, 1000),
                    HAL_OK);

  return true;
}

bool gt911_write(uint16_t reg_addr, uint8_t* buf, uint16_t len) {
  // write
  GT911_ECA_R_FALSE(HAL_I2C_Mem_Write(i2c_handle_touchpanel, GT911_ADDR,
                                      reg_addr, 2, buf, len, 1000),
                    HAL_OK);

  // handle config regs and checksum
  if (((reg_addr >= GTP_REG_CONF_VER) &&
       (reg_addr < GTP_REG_CONF_CHANGE_NOTIFY)) ||
      ((reg_addr + len - 1) >= GTP_REG_CONF_VER)) {
    uint8_t reg_conf_data[GTP_REG_CONF_CHANGE_NOTIFY - GTP_REG_CONF_VER - 1];
    uint8_t calc_chksum = 0U;
    uint8_t chip_chksum = 0xffU;
    uint8_t notify = 1U;

    // read back whole config
    GT911_ECA_R_FALSE(
        gt911_read(GTP_REG_CONF_VER, reg_conf_data, sizeof(reg_conf_data)),
        true);

    // calculate checksum
    for (size_t i = 0; i < sizeof(reg_conf_data); i++) {
      calc_chksum += reg_conf_data[i];
    }
    calc_chksum = ((~calc_chksum) + 1U) & 0xffU;

    // read back chip side checksum
    GT911_ECA_R_FALSE(gt911_read(GTP_REG_CONF_CHKSUM, &chip_chksum, 1), true);

    if (calc_chksum != chip_chksum) {
      GT911_ECA_R_FALSE(
          HAL_I2C_Mem_Write(i2c_handle_touchpanel, GT911_ADDR,
                            GTP_REG_CONF_CHKSUM, 2, &calc_chksum, 1, 1000),
          HAL_OK);
      GT911_ECA_R_FALSE(
          HAL_I2C_Mem_Write(i2c_handle_touchpanel, GT911_ADDR,
                            GTP_REG_CONF_CHANGE_NOTIFY, 2, &notify, 1, 1000),
          HAL_OK);
    }
  }

  return true;
}

// return one point data only
uint32_t gt911_read_location(void) {
  uint8_t point_data[10] = {0};
  uint8_t point_num;
  uint16_t x = 0, y = 0;
  static uint32_t xy = 0;

  static uint8_t last_point_num = 0;

  ensure((gt911_read(GTP_READ_COOR_ADDR, point_data, 10) ? sectrue : secfalse),
         "gt911_read error");
  if (point_data[0] == 0x00) {
    return xy;
  }

  if (point_data[0] == 0x80) {
    point_data[0] = 0;
    ensure(
        (gt911_write(GTP_READ_COOR_ADDR, point_data, 1) ? sectrue : secfalse),
        "gt911_write error");
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
  ensure((gt911_write(GTP_READ_COOR_ADDR, point_data, 1) ? sectrue : secfalse),
         "gt911_write error");

  xy = x << 16 | y;

  return xy;
}

void gt911_enter_sleep(void) {
  uint8_t data[1] = {0x05};
  ensure((gt911_write(GTP_REG_COMMAND, data, 1) ? sectrue : secfalse),
         "gt911_write error");
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

void gt911_set_config(void) {
  uint8_t config_data[sizeof(GT911_Config_t)] = {0};
  GT911_Config_t* p_config = (GT911_Config_t*)config_data;

  gt911_read(GTP_REG_CONFIG_DATA, (uint8_t *)config_data, 1);
  if (config_data[0] == 0x50) {
    return;
  }

  ensure((gt911_read(GTP_REG_CONFIG_DATA, (uint8_t*)config_data,
                     sizeof(config_data))
              ? sectrue
              : secfalse),
         "gt911_read error");

  p_config->config_version = 0x50;

  p_config->shake_count = 0x11;
  p_config->noise_reduction = 10;
  p_config->screen_touch_level = 0x60;

  p_config->check_sum = 0;
  for (int i = 0; i < sizeof(config_data) - 2; i++) {
    p_config->check_sum += config_data[i];
  }
  p_config->check_sum = (~p_config->check_sum) + 1;
  p_config->config_refresh = 0x01;

  ensure((gt911_write(GTP_REG_CONFIG_DATA, (uint8_t*)config_data,
                      sizeof(config_data))
              ? sectrue
              : secfalse),
         "gt911_write error");
}

void gt911_init(void) {
  i2c_handle_touchpanel =
      &i2c_handles[i2c_find_channel_by_device(I2C_TOUCHPANEL)];
  gt911_io_init();
  gt911_reset();
  i2c_init_by_device(I2C_TOUCHPANEL);

  gt911_set_config();
}

bool FUN_NO_OPTMIZE gt911_wait_gesture(GTP_GESTURE_INFO_t* gi,
                                       GTP_GESTURE_STATUS_t* gs) {
  // disable irq
  gt911_disable_irq();

  // reset
  gt911_reset();

  // module switch 3
  // GT911_ECA_R_FALSE(gt911_read(GTP_REG_CONF_MODSWITCH3, gt911_buffer, 1),
  // true); gt911_buffer[0] |= 0x02;
  // GT911_ECA_R_FALSE(gt911_write(GTP_REG_CONF_MODSWITCH3, gt911_buffer, 1),
  // true);

  // config gesture mode
  gt911_buffer[0] = 0x0f;
  GT911_ECA_R_FALSE(gt911_write(GTP_REG_CONF_REFRESH_RATE, gt911_buffer, 1),
                    true);
  gt911_buffer[0] = 0x00;
  GT911_ECA_R_FALSE(gt911_write(GTP_REG_CONF_GEST_LG_TOUCH, gt911_buffer, 1),
                    true);

  GTP_CONF_GESTURE_t gestrure_conf = {0};
  GT911_ECA_R_FALSE(
      gt911_read(GTP_REG_CONF_GEST_DIST, (uint8_t*)(&gestrure_conf),
                 sizeof(GTP_CONF_GESTURE_t)),
      true);
  // gestrure_conf.slide_distance = 0x55;
  // gestrure_conf.long_press_time = 0x00;
  // gestrure_conf.xy_slope_adj = 0x88;
  gestrure_conf.control = 0x0fU;
  gestrure_conf.sw1 = 0xff;
  gestrure_conf.sw2 = 0xff;
  // gestrure_conf.refresh_rate = 0xff;
  // gestrure_conf.threshold = 0;
  GT911_ECA_R_FALSE(
      gt911_write(GTP_REG_CONF_GEST_DIST, (uint8_t*)&gestrure_conf, 1), true);

  // enter gesture mode
  gt911_buffer[0] = 0x08;
  gt911_write(GTP_REG_RT_CMDCK, gt911_buffer, 1);
  gt911_write(GTP_REG_RT_CMD, gt911_buffer, 1);

  while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET) {
    hal_delay(1);
  }

  hal_delay(4);  // wait 300us

  GT911_ECA_R_FALSE(gt911_read(GTP_REG_GEST_GID_B0, (uint8_t*)(gi),
                               sizeof(GTP_GESTURE_INFO_t)),
                    true);
  GT911_ECA_R_FALSE(gt911_read(GTP_REG_GEST_TYPE, (uint8_t*)(gs),
                               sizeof(GTP_GESTURE_STATUS_t)),
                    true);

  gt911_reset();  // reset to exit gesture mode

  if (gs->g_type == 0x00)  // check type here, dummy
    return false;

  return true;
}

void FUN_NO_OPTMIZE gt911_test_gesture(void) {
  GTP_GESTURE_INFO_t gi;
  GTP_GESTURE_STATUS_t gs;
  uint16_t sx, sy, ex, ey;

  display_clear();

  while (1) {
    display_print_clear();
    if (gt911_wait_gesture(&gi, &gs)) {
      display_printf("GID = %c%c%c%c \n", gi.g_id[0], gi.g_id[1], gi.g_id[2],
                     gi.g_id[3]);
      display_printf("fw_ver = 0x%04x \n", gi.fw_ver);
      sx = BIGE_16(gs.g_cord_start_x);
      sy = BIGE_16(gs.g_cord_start_y);
      ex = BIGE_16(gs.g_cord_end_x);
      ey = BIGE_16(gs.g_cord_end_y);
      display_printf("type = %u \n", gs.g_type);
      display_printf("start x = %u y = %u \n", sx, sy);
      display_printf("end x = %u y = %u \n", ex, ex);
      display_bar(sx, sy, 30, 30, COLOR_RED);
      display_bar(ex, ey, 30, 30, COLOR_GRAY);

      hal_delay(3000);
    } else {
      display_printf("GID = %c%c%c%c \n", gi.g_id[0], gi.g_id[1], gi.g_id[2],
                     gi.g_id[3]);
      display_printf("fw_ver = 0x%04x \n", gi.fw_ver);
      display_printf("type = %u \n", gs.g_type);
    }
    hal_delay(50);
  }
}