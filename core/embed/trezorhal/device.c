#include STM32_HAL_H

#include "device.h"
#include "common.h"
#include "display.h"
#include "emmc.h"
#include "flash.h"
#include "mini_printf.h"
#include "mipi_lcd.h"
#include "motor.h"
#include "qspi_flash.h"
#include "rand.h"
#include "se_thd89.h"
#include "secp256k1.h"
#include "sha2.h"
#include "systick.h"
#include "thd89.h"
#include "touch.h"

#include "ble.h"
#include "camera.h"
#include "ff.h"
#include "fingerprint.h"
#include "fp_sensor_wrapper.h"
#include "hardware_version.h"
#include "jpeg_dma.h"
#include "nfc.h"
#include "sdram.h"
#include "usart.h"

#include "emmc_fs.h"

static DeviceInfomation dev_info = {0};
static bool serial_set = false;
static bool factory_mode = false;

static bool is_valid_ascii(const uint8_t *data, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    if (data[i] == 0) {
      break;
    }
    if (data[i] < ' ' || data[i] > '~') {
      return false;
    }
  }
  return true;
}

void device_set_factory_mode(bool mode) { factory_mode = mode; }

bool device_is_factory_mode(void) { return factory_mode; }

void device_verify_ble(void) {
  uint8_t pubkey[65];
  uint8_t rand_buf[16], signature[64], digest[32];
  char *ble_ver;
  char info[64] = {0};
  ensure(ble_get_version(&ble_ver) ? sectrue : secfalse, NULL);
  if (memcmp(ble_ver, "2.2.3", 5) < 0) {
    strcat(info, "current ble version: ");
    strcat(info, ble_ver);
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2,
                        "Please upgrade the Bluetooth.", -1, FONT_NORMAL,
                        COLOR_WHITE, COLOR_BLACK);
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2 + 30,
                        "Touch the screen to continue", -1, FONT_NORMAL,
                        COLOR_WHITE, COLOR_BLACK);
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2 + 60, info, -1,
                        FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
    while (!touch_click())
      ;
    return;
  }

  if (secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_BLE_PUBKEY1) ||
      secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_BLE_PUBKEY2)) {
    ensure(ble_get_pubkey(pubkey + 1) ? sectrue : secfalse, NULL);
    ensure(ble_lock_pubkey() ? sectrue : secfalse, NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_BLE_PUBKEY1, 0, pubkey + 1,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_BLE_PUBKEY2, 0, pubkey + 33,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_BLE_PUBKEY1), NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_BLE_PUBKEY2), NULL);
  } else {
    ensure(flash_otp_read(FLASH_OTP_BLOCK_BLE_PUBKEY1, 0, pubkey + 1,
                          FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_read(FLASH_OTP_BLOCK_BLE_PUBKEY2, 0, pubkey + 33,
                          FLASH_OTP_BLOCK_SIZE),
           NULL);
  }
  pubkey[0] = 0x04;
  random_buffer(rand_buf, sizeof(rand_buf));
  ensure(
      ble_sign_msg(rand_buf, sizeof(rand_buf), signature) ? sectrue : secfalse,
      NULL);
  sha256_Raw(rand_buf, 16, digest);

  ensure(ecdsa_verify_digest(&secp256k1, pubkey, signature, digest) == 0
             ? sectrue
             : secfalse,
         NULL);
}

void device_para_init(void) {
  dev_info.st_id[0] = HAL_GetUIDw0();
  dev_info.st_id[1] = HAL_GetUIDw1();
  dev_info.st_id[2] = HAL_GetUIDw2();

  uint8_t pubkey[64];
  if (secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_1_PUBKEY1) ||
      secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_1_PUBKEY2)) {
    ensure(se_get_ecdh_pubkey(THD89_MASTER_ADDRESS, pubkey), NULL);
    ensure(se_lock_ecdh_pubkey(THD89_MASTER_ADDRESS), NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_1_PUBKEY1, 0, pubkey,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_1_PUBKEY2, 0, pubkey + 32,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_1_PUBKEY1), NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_1_PUBKEY2), NULL);
  }

  if (secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_2_PUBKEY1) ||
      secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_2_PUBKEY2)) {
    ensure(se_get_ecdh_pubkey(THD89_2ND_ADDRESS, pubkey), NULL);
    ensure(se_lock_ecdh_pubkey(THD89_2ND_ADDRESS), NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_2_PUBKEY1, 0, pubkey,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_2_PUBKEY2, 0, pubkey + 32,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_2_PUBKEY1), NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_2_PUBKEY2), NULL);
  }

  if (secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_3_PUBKEY1) ||
      secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_3_PUBKEY2)) {
    ensure(se_get_ecdh_pubkey(THD89_3RD_ADDRESS, pubkey), NULL);
    ensure(se_lock_ecdh_pubkey(THD89_3RD_ADDRESS), NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_3_PUBKEY1, 0, pubkey,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_3_PUBKEY2, 0, pubkey + 32,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_3_PUBKEY1), NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_3_PUBKEY2), NULL);
  }

  if (secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_4_PUBKEY1) ||
      secfalse == flash_otp_is_locked(FLASH_OTP_BLOCK_THD89_4_PUBKEY2)) {
    ensure(se_get_ecdh_pubkey(THD89_FINGER_ADDRESS, pubkey), NULL);
    ensure(se_lock_ecdh_pubkey(THD89_FINGER_ADDRESS), NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_4_PUBKEY1, 0, pubkey,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_write(FLASH_OTP_BLOCK_THD89_4_PUBKEY2, 0, pubkey + 32,
                           FLASH_OTP_BLOCK_SIZE),
           NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_4_PUBKEY1), NULL);
    ensure(flash_otp_lock(FLASH_OTP_BLOCK_THD89_4_PUBKEY2), NULL);
  }

  if (!flash_otp_is_locked(FLASH_OTP_RANDOM_KEY)) {
    uint8_t entropy[FLASH_OTP_BLOCK_SIZE] = {0};
    random_buffer(entropy, FLASH_OTP_BLOCK_SIZE);
    ensure(
        flash_otp_write(FLASH_OTP_RANDOM_KEY, 0, entropy, FLASH_OTP_BLOCK_SIZE),
        NULL);
    ensure(flash_otp_lock(FLASH_OTP_RANDOM_KEY), NULL);
  }
  dev_info.random_key_init = true;
  ensure(flash_otp_read(FLASH_OTP_RANDOM_KEY, 0, dev_info.random_key,
                        FLASH_OTP_BLOCK_SIZE),
         NULL);

  if (flash_otp_is_locked(FLASH_OTP_CPU_FIRMWARE_INFO)) {
    strlcpy(dev_info.cpu_info,
            (char *)flash_otp_data->flash_otp[FLASH_OTP_CPU_FIRMWARE_INFO],
            sizeof(dev_info.cpu_info));
    strlcpy(dev_info.pre_firmware,
            (char *)(flash_otp_data->flash_otp[FLASH_OTP_CPU_FIRMWARE_INFO] +
                     FLASH_OTP_BLOCK_SIZE / 2),
            sizeof(dev_info.pre_firmware));
  }

  if (!flash_otp_is_locked(FLASH_OTP_DEVICE_SERIAL)) {
    serial_set = false;
    return;
  }

  strlcpy(dev_info.serial,
          (char *)flash_otp_data->flash_otp[FLASH_OTP_DEVICE_SERIAL],
          sizeof(dev_info.serial));

  if (is_valid_ascii((uint8_t *)dev_info.serial, FLASH_OTP_BLOCK_SIZE)) {
    serial_set = true;
  }

  return;
}

bool device_serial_set(void) { return serial_set; }

bool device_set_serial(char *dev_serial) {
  uint8_t buffer[FLASH_OTP_BLOCK_SIZE] = {0};

  if (serial_set) {
    return false;
  }

  if (!is_valid_ascii((uint8_t *)dev_serial, FLASH_OTP_BLOCK_SIZE - 1)) {
    return false;
  }

  // check serial
  if (!flash_otp_is_locked(FLASH_OTP_DEVICE_SERIAL)) {
    if (check_all_ones(flash_otp_data->flash_otp[FLASH_OTP_DEVICE_SERIAL],
                       FLASH_OTP_BLOCK_SIZE)) {
      strlcpy((char *)buffer, dev_serial, sizeof(buffer));
      ensure(flash_otp_write(FLASH_OTP_DEVICE_SERIAL, 0, buffer,
                             FLASH_OTP_BLOCK_SIZE),
             NULL);
      ensure(flash_otp_lock(FLASH_OTP_DEVICE_SERIAL), NULL);
      return true;
    }
  }
  return false;
}

bool device_get_serial(char **serial) {
  if (!serial_set) {
    return false;
  }
  *serial = dev_info.serial;
  return true;
}

bool device_cpu_firmware_set(void) {
  if ((0 < strlen(dev_info.cpu_info) &&
       strlen(dev_info.cpu_info) < FLASH_OTP_BLOCK_SIZE / 2) &&
      (0 < strlen(dev_info.pre_firmware) &&
       strlen(dev_info.pre_firmware) < FLASH_OTP_BLOCK_SIZE / 2)) {
    return true;
  }
  return false;
}

bool device_set_cpu_firmware(char *cpu_info, char *firmware_ver) {
  uint8_t buffer[FLASH_OTP_BLOCK_SIZE] = {0};

  // check serial
  if (!flash_otp_is_locked(FLASH_OTP_CPU_FIRMWARE_INFO)) {
    if (check_all_ones(flash_otp_data->flash_otp[FLASH_OTP_CPU_FIRMWARE_INFO],
                       FLASH_OTP_BLOCK_SIZE)) {
      strlcpy((char *)buffer, cpu_info, sizeof(buffer) / 2);
      strlcpy((char *)buffer + FLASH_OTP_BLOCK_SIZE / 2, firmware_ver,
              sizeof(buffer) / 2);
      ensure(flash_otp_write(FLASH_OTP_CPU_FIRMWARE_INFO, 0, buffer,
                             FLASH_OTP_BLOCK_SIZE),
             NULL);
      ensure(flash_otp_lock(FLASH_OTP_CPU_FIRMWARE_INFO), NULL);
      device_para_init();
      return true;
    }
  }
  return false;
}

bool device_get_cpu_firmware(char **cpu_info, char **firmware_ver) {
  if (device_cpu_firmware_set()) {
    *cpu_info = dev_info.cpu_info;
    *firmware_ver = dev_info.pre_firmware;
  }
  return false;
}

void device_get_enc_key(uint8_t key[32]) {
  SHA256_CTX ctx = {0};

  sha256_Init(&ctx);
  sha256_Update(&ctx, (uint8_t *)dev_info.st_id, sizeof(dev_info.st_id));
  sha256_Update(&ctx, dev_info.random_key, sizeof(dev_info.random_key));
  sha256_Final(&ctx, key);
}

void ui_test_input(void) {
  display_clear();
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 6; j++) {
      display_bar_radius(j * 80, (j % 2) * 80 + i * 160, 80, 80, COLOR_RED,
                         COLOR_WHITE, 16);
    }
  }
  uint32_t pos = 0;
  for (;;) {
    uint32_t evt = touch_read();
    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);

    if (!evt) {
      continue;
    }

    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 6; j++) {
        if (x > (j * 80) && x < (j * 80 + 80) && y > ((j % 2) * 80 + i * 160) &&
            y < ((j % 2) * 80 + i * 160 + 80)) {
          display_bar_radius(j * 80, (j % 2) * 80 + i * 160, 80, 80,
                             COLOR_GREEN, COLOR_WHITE, 16);
          pos |= 1 << (6 * i + j);
        }
        if (pos == 0x3FFFFFFF) {
          return;
        }
      }
    }
  }
}

void device_burnin_test_clear_flag(void) {
  FRESULT res;
  FIL fil;

  UINT bw;
  test_result test_res = {0};

  FATFS fs;

  res = f_mount(&fs, "", 1);
  if (res != FR_OK) {
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2,
                        "mount fatfs failed", -1, FONT_NORMAL, COLOR_RED,
                        COLOR_BLACK);
    while (1)
      ;
  }

  f_open(&fil, "test_res", FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
  f_write(&fil, &test_res, sizeof(test_res), &bw);
  f_sync(&fil);
  hal_delay(100);
  HAL_NVIC_SystemReset();
}

#if BOOT_ONLY
#include "bootui.h"
void device_generate_trng_data(void) {
  // TRNG test
  display_clear();

  // var
  char title_buf[] = "TRNG Generate\0";
  char note_buf[32];
  char path_buf[FF_MAX_LFN];
  uint8_t se_rand_buffer[1024];  // 1024 max supported by SE
  uint32_t processed_len;

  const uint8_t batch_total = 2;
  uint8_t batch_current = 1;

  const uint32_t batch_total_bytes = 10 * 1024 * 1024;  // 10MB
  uint32_t batch_processed_bytes = 0;

  // rmdir
  ensure_emmcfs(emmc_fs_dir_delete("0:TRNG_Test_Data"), "rmdir failed");

  // mkdir
  ensure_emmcfs(emmc_fs_dir_make("0:TRNG_Test_Data"), "mkdir failed");

  // loop
  while (batch_current <= batch_total) {
    // ui init
    snprintf(note_buf, sizeof(note_buf), "Batch   %u / %u", batch_current,
             batch_total);
    display_clear();
    ui_screen_progress_bar_prepare(title_buf, note_buf);

    // path
    snprintf(path_buf, sizeof(path_buf), "0:TRNG_Test_Data/batch_%u.bin",
             batch_current);

    // batch
    batch_processed_bytes = 0;
    while (batch_processed_bytes < batch_total_bytes) {
      // get trng
      se_get_rand(se_rand_buffer, sizeof(se_rand_buffer));

      // write to file
      ensure_emmcfs(emmc_fs_file_write(path_buf, batch_processed_bytes,
                                       se_rand_buffer, sizeof(se_rand_buffer),
                                       &processed_len, false, true),
                    "file write failed");
      // EMMC_WRAPPER_UNUSED(processed_len);

      // update progress
      batch_processed_bytes += sizeof(se_rand_buffer);

      // ui update
      ui_screen_progress_bar_update(
          NULL, NULL, (batch_processed_bytes * 100 / batch_total_bytes));

      // delay
      // hal_delay(10);
    }

    // reset se each batch
    se_reset_se();

    batch_current++;
  }

  // ui update (last)
  ui_screen_progress_bar_update(NULL, NULL, 100);

  // ui done
  display_clear();
  ui_screen_success("Finished", "Click to go back to main menu.");
  while (!touch_click()) {
  }
  display_clear();
  ui_bootloader_first(NULL);
}
#endif

#if DEVICE_TEST

#include "lite_card.h"

static FATFS fs_instance;

typedef enum {
  TEST_NULL = 0x00000000,
  TEST_TESTING = 0x11111111,
  TEST_PASS = 0x22222222,
  TEST_FAILED = 0x33333333
} test_status;

enum { UI_RESPONSE_NONE = 0, UI_RESPONSE_YES, UI_RESPONSE_NO };

static TIM_HandleTypeDef TimHandle;

static void test_timer_init(void) {
  __HAL_RCC_TIM2_CLK_ENABLE();
  TimHandle.Instance = TIM2;
  TimHandle.Init.Prescaler = (uint32_t)(SystemCoreClock / (2 * 10000)) - 1;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.Period = 0xffffffff;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  TimHandle.Init.RepetitionCounter = 0;
  HAL_TIM_Base_Init(&TimHandle);
  HAL_TIM_Base_Start(&TimHandle);
}

static void lock_burnin_test_otp(void) {
  uint8_t buf[FLASH_OTP_BLOCK_SIZE] = {0};
  memcpy(buf, "device burn-in test done", strlen("device burn-in test done"));
  ensure(flash_otp_write(FLASH_OTP_BLOCK_BURNIN_TEST, 0, buf,
                         FLASH_OTP_BLOCK_SIZE),
         NULL);
  ensure(flash_otp_lock(FLASH_OTP_BLOCK_BURNIN_TEST), NULL);
}

#define TIMER_1S 10000
#define TEST_DURATION (3 * 60 * 60 * TIMER_1S)  // 3 hours

static void ui_generic_confirm_simple(const char *msg) {
  if (msg == NULL) return;
  display_bar(0, 380, DISPLAY_RESX, 420, COLOR_BLACK);
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, msg, -1, FONT_NORMAL,
                      COLOR_WHITE, COLOR_BLACK);

  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
                     COLOR_GREEN, COLOR_BLACK, 16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);
  display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1, FONT_NORMAL,
               COLOR_WHITE, COLOR_GREEN);
}

static bool ui_response(void) {
  for (;;) {
    uint32_t evt = touch_click();
    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);

    if (!evt) {
      continue;
    }
    // clicked on Cancel button
    if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
        y < DISPLAY_RESY - 160 + 64) {
      return false;
    }
    // clicked on Confirm button
    if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
        y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
      return true;
    }
  }
}

static int ui_response_ex(void) {
  uint32_t evt = touch_click();

  if (!evt) {
    return UI_RESPONSE_NONE;
  }
  uint16_t x = touch_unpack_x(evt);
  uint16_t y = touch_unpack_y(evt);
  // clicked on Cancel button
  if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
      y < DISPLAY_RESY - 160 + 64) {
    return UI_RESPONSE_NO;
  }
  // clicked on Confirm button
  if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
      y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
    return UI_RESPONSE_YES;
  }
  return UI_RESPONSE_NONE;
}

static bool _motor_test(void) {
  int ui_res = 0;
  motor_init();
  ui_generic_confirm_simple("MOTOR test");

  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_SET);
  while (1) {
    HAL_GPIO_WritePin(GPIOK, GPIO_PIN_2, GPIO_PIN_RESET);
    dwt_delay_us(2083);
    HAL_GPIO_WritePin(GPIOK, GPIO_PIN_2, GPIO_PIN_SET);
    dwt_delay_us(767);

    ui_res = ui_response_ex();
    if (ui_res != UI_RESPONSE_NONE) {
      return ui_res == UI_RESPONSE_YES;
    }
  }
}

static bool _camera_test(void) {
  int ui_res = 0;
  while (1) {
    camera_capture_start();
    if (camera_capture_done()) {
      dma2d_copy_buffer((uint32_t *)CAM_BUF_ADDRESS,
                        (uint32_t *)FMC_SDRAM_LTDC_BUFFER_ADDRESS, 80, 240,
                        WIN_W, WIN_H);
    }

    ui_res = ui_response_ex();
    if (ui_res != UI_RESPONSE_NONE) {
      display_bar(80, 240, WIN_W, WIN_H, COLOR_BLACK);
      return ui_res == UI_RESPONSE_YES;
    }
  }
}

static bool _nfc_test() {
  int ui_res = 0;
  display_bar(0, 380, DISPLAY_RESX, 420, COLOR_BLACK);
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "NFC test", -1,
                      FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);

  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);

  nfc_init();
  nfc_pwr_ctl(true);
  while (1) {
    if (nfc_poll_card()) {
      if (nfc_select_aid((uint8_t *)"\xD1\x56\x00\x01\x32\x83\x40\x01", 8)) {
        if (lite_card_data_exchange_test()) {
          nfc_pwr_ctl(false);
          return true;
        }
      }

      if (nfc_select_aid(
              (uint8_t
                   *)"\x6f\x6e\x65\x6b\x65\x79\x2e\x62\x61\x63\x6b\x75\x70\x01",
              14)) {
        if (lite_card_data_exchange_test()) {
          nfc_pwr_ctl(false);
          return true;
        }
      }
    }

    ui_res = ui_response_ex();
    if (ui_res == UI_RESPONSE_NO) {
      nfc_pwr_ctl(false);
      return false;
    }
  }
  return false;
}

static bool _flashled_test(void) {
  uint32_t start, current, value;
  int ui_res = 0;
  start = current = HAL_GetTick();
  value = 1;
  ui_generic_confirm_simple("FLASHLED test");
  ble_set_flashled(value);
  while (1) {
    current = HAL_GetTick();
    if (current - start > 1000) {
      start = current;
      value = value ? 0 : 1;
      ble_set_flashled(value);
    }
    ui_res = ui_response_ex();
    if (ui_res != UI_RESPONSE_NONE) {
      ble_set_flashled(0);
      return ui_res == UI_RESPONSE_YES;
    }
  }
}

static bool _fp_test(void) {
  int ui_res = 0;
  uint8_t image_data[88 * 112 + 2];
  ui_generic_confirm_simple("FINGERPRINT test");
  while (1) {
    if (FpsDetectFinger() == 1) {
      if (FpsGetImageData(image_data) == 0) {
        display_fp(196, 500, 88, 112, image_data);
      }
    }
    ui_res = ui_response_ex();
    if (ui_res != UI_RESPONSE_NONE) {
      return ui_res == UI_RESPONSE_YES;
    }
  }
}

static bool _sdram_test(void) {
  uint32_t *sdram_addr = (uint32_t *)(FMC_SDRAM_ADDRESS + 1024 * 1024);
  uint32_t i = 0;
  while (1) {
    for (i = 0; i < 31 * 1024 * 1024 / 4; i++) {
      sdram_addr[i] = i;
    }
    for (i = 0; i < 31 * 1024 * 1024 / 4; i++) {
      if (sdram_addr[i] != i) {
        return false;
      }
    }
    return true;
  }
}

void device_test(bool force) {
  if (flash_otp_is_locked(FLASH_OTP_FACTORY_TEST) && !force) {
    return;
  }
  display_bar(0, 0, MAX_DISPLAY_RESX, 266, COLOR_RED);
  display_bar(0, 266, MAX_DISPLAY_RESX, 532, COLOR_GREEN);
  display_bar(0, 532, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, COLOR_BLUE);

  while (!touch_click()) {
  }

  ui_test_input();

  display_clear();

  uint8_t rand_buffer[32];

  if (!se_get_rand(rand_buffer, 32)) {
    display_text(0, 20, "SE test faild", -1, FONT_NORMAL, COLOR_RED,
                 COLOR_BLACK);
    while (1)
      ;
  } else {
    display_text(0, 20, "SE test done", -1, FONT_NORMAL, COLOR_WHITE,
                 COLOR_BLACK);
  }

  if (get_hw_ver() < HW_VER_3P0A) {
    if (qspi_flash_read_id() == 0) {
      display_text(0, 50, "SPI-FLASH test faild", -1, FONT_NORMAL, COLOR_RED,
                   COLOR_BLACK);
      while (1)
        ;
    } else {
      display_text(0, 50, "SPI-FLASH test done", -1, FONT_NORMAL, COLOR_WHITE,
                   COLOR_BLACK);
    }
  }

  if (emmc_get_capacity_in_bytes() == 0) {
    display_text(0, 80, "EMMC test faild", -1, FONT_NORMAL, COLOR_RED,
                 COLOR_BLACK);
    while (1)
      ;
  } else {
    display_text(0, 80, "EMMC test done", -1, FONT_NORMAL, COLOR_WHITE,
                 COLOR_BLACK);
  }

  if (_motor_test()) {
    display_text(0, 110, "MOTOR test done", -1, FONT_NORMAL, COLOR_WHITE,
                 COLOR_BLACK);
  } else {
    display_text(0, 110, "MOTOR test faild", -1, FONT_NORMAL, COLOR_RED,
                 COLOR_BLACK);
    while (1)
      ;
  }

  if (_camera_test()) {
    display_text(0, 140, "CAMERA test done", -1, FONT_NORMAL, COLOR_WHITE,
                 COLOR_BLACK);
  } else {
    display_text(0, 140, "CAMERA test faild", -1, FONT_NORMAL, COLOR_RED,
                 COLOR_BLACK);
    while (1)
      ;
  }

  if (_nfc_test()) {
    display_text(0, 170, "NFC test done", -1, FONT_NORMAL, COLOR_WHITE,
                 COLOR_BLACK);
  } else {
    display_text(0, 170, "NFC test faild", -1, FONT_NORMAL, COLOR_RED,
                 COLOR_BLACK);
    while (1)
      ;
  }

  if (_fp_test()) {
    display_text(0, 200, "FINGERPRINT test done", -1, FONT_NORMAL, COLOR_WHITE,
                 COLOR_BLACK);
  } else {
    display_text(0, 200, "FINGERPRINT test faild", -1, FONT_NORMAL, COLOR_RED,
                 COLOR_BLACK);
    while (1)
      ;
  }

  if (_flashled_test()) {
    display_text(0, 230, "FLASHLED test done", -1, FONT_NORMAL, COLOR_WHITE,
                 COLOR_BLACK);
  } else {
    display_text(0, 230, "FLASHLED test faild", -1, FONT_NORMAL, COLOR_RED,
                 COLOR_BLACK);
    while (1)
      ;
  }

  uint8_t buf[FLASH_OTP_BLOCK_SIZE] = {0};
  memcpy(buf, "test passed", strlen("test passed"));
  ensure(flash_otp_write(FLASH_OTP_FACTORY_TEST, 0, buf, FLASH_OTP_BLOCK_SIZE),
         NULL);
  ensure(flash_otp_lock(FLASH_OTP_FACTORY_TEST), NULL);

  if (!force) {
    char count_str[24] = {0};
    for (int i = 1; i >= 0; i--) {
      display_bar(0, 230, DISPLAY_RESX, 30, COLOR_BLACK);
      mini_snprintf(count_str, sizeof(count_str), "Done! Restarting in %d s",
                    i);
      display_text(0, 260, count_str, -1, FONT_NORMAL, COLOR_WHITE,
                   COLOR_BLACK);
      hal_delay(1000);
    }
    HAL_NVIC_SystemReset();
  }
}

void device_burnin_test(bool force) {
  uint32_t start = 0, current, remain, previous_remain, previous;
  uint8_t rand_buffer[32];
  char remain_timer[16] = {0};

  volatile uint64_t emmc_cap = 0;
  volatile uint32_t flash_id = 0;
  volatile uint32_t index = 0, index_bak = 0xff;
  volatile uint32_t click = 0, click_pre = 0, click_now = 0;
  volatile uint32_t flashled_pre = 0, flashled_now = 0;
  volatile bool fingerprint_detect = false;
  static uint8_t charge_type_last = 0xff;
  static uint16_t voltage_last = 0, current_last = 0,
                  discahrging_current_last = 0;
  volatile uint32_t battery_pre = 0, battery_now = 0;
  uint8_t image_data[88 * 112 + 2];
  int flashled_value = 1;

  FRESULT res;
  FIL fil;

  UINT br, bw;
  test_result test_res = {0};

  res = f_mount(&fs_instance, "", 1);
  if (res != FR_OK) {
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2,
                        "mount fatfs failed", -1, FONT_NORMAL, COLOR_RED,
                        COLOR_BLACK);
    while (1)
      ;
  }

  f_open(&fil, "test_res", FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
  f_chmod("test_res", AM_SYS | AM_HID, AM_SYS | AM_HID);

  if (force) {
    f_write(&fil, &test_res, sizeof(test_res), &bw);
    f_sync(&fil);
  }

  f_read(&fil, &test_res, sizeof(test_res), &br);
  if (br == 0) {
    test_res.flag = TEST_TESTING;
    test_res.time = 0;
    f_write(&fil, &test_res, sizeof(test_res), &bw);
    f_sync(&fil);
  }
  if (flash_otp_is_locked(FLASH_OTP_BLOCK_BURNIN_TEST) &&
      (test_res.touch == TEST_PASS)) {
    return;
  }

  if (test_res.flag == TEST_TESTING) {
    start = test_res.time;
  } else if (test_res.flag == TEST_PASS) {
    if (test_res.touch != TEST_PASS) {
      device_test(true);

      test_res.touch = TEST_PASS;
      f_lseek(&fil, 0);
      f_write(&fil, &test_res, sizeof(test_res), &bw);
      f_sync(&fil);
      ble_cmd_req(BLE_BT, BLE_BT_ON);
      lock_burnin_test_otp();
      restart();
    }
    return;
  }

  test_timer_init();
  jpeg_init();
  motor_init();

  nfc_init();

  previous_remain = 0;
  previous = 0;

  flashled_pre = flashled_now = HAL_GetTick();
  battery_pre = battery_now = HAL_GetTick();

  ble_set_flashled(flashled_value);
  ble_cmd_req(BLE_PWR, BLE_PWR_CHARGE_ENABLE);

  do {
    ble_uart_poll();
    if (touch_click()) {
      hal_delay(50);
      if (click == 0) {
        click_now = __HAL_TIM_GET_COUNTER(&TimHandle);
      }
      click++;
      click_pre = click_now;
      click_now = __HAL_TIM_GET_COUNTER(&TimHandle);

      if (click_now - click_pre > (TIMER_1S / 2)) {
        click = 0;
      }
      if (click == 3) {
        click = 0;
        display_clear();
        HAL_TIM_Base_Stop(&TimHandle);
        ui_generic_confirm_simple("EXIT  TEST?");
        if (ui_response()) {
          ble_cmd_req(BLE_BT, BLE_BT_ON);
          test_res.flag = TEST_PASS;
          f_lseek(&fil, 0);
          f_write(&fil, &test_res, sizeof(test_res), &bw);
          f_sync(&fil);
          restart();
        } else {
          click = 0;
          index_bak = 0xff;
          HAL_TIM_Base_Start(&TimHandle);
        }
      }
    }
    current = start + __HAL_TIM_GET_COUNTER(&TimHandle);

    remain = TEST_DURATION - current;
    if (previous_remain == 0) {
      previous_remain = remain;
    }

    if (previous_remain - remain >= (TIMER_1S / 2)) {
      previous_remain = remain;
      remain /= TIMER_1S;
      uint8_t hour = remain / 3600;
      uint8_t min = (remain % 3600) / 60;
      uint8_t sec = (remain % 3600) % 60;

      int w = display_text_width(remain_timer, -1, FONT_NORMAL);
      mini_snprintf(remain_timer, sizeof(remain_timer), "%02d:%02d:%02d", hour,
                    min, sec);
      display_bar(DISPLAY_RESX / 2 - w / 2, 770, w, 30, COLOR_BLACK);
      display_text_center(DISPLAY_RESX / 2, 800, remain_timer, -1, FONT_NORMAL,
                          COLOR_WHITE, COLOR_BLACK);
    }

    index = (current / (TIMER_1S * 3)) % 4;
    if (index != index_bak) {
      index_bak = index;
      switch (index) {
        case 0:
          // display_bar(0, 0, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, COLOR_RED);
          if (jped_decode("res/wallpaper-1.jpg",
                          FMC_SDRAM_LVGL_BUFFER_ADDRESS) != 0) {
            display_print_clear();
            display_printf("show wallpaper-1.jpg err\n");
          }
          break;
        case 1:
          if (jped_decode("res/wallpaper-2.jpg",
                          FMC_SDRAM_LVGL_BUFFER_ADDRESS) != 0) {
            display_print_clear();
            display_printf("show wallpaper-2.jpg err\n");
          }
          break;
        case 2:
          if (jped_decode("res/wallpaper-3.jpg",
                          FMC_SDRAM_LVGL_BUFFER_ADDRESS) != 0) {
            display_print_clear();
            display_printf("show wallpaper-3.jpg err\n");
          }
          break;
        case 3:
          display_clear();
          display_print_clear();

          display_printf("SPI_FLASH ID= 0x%X \n", (unsigned)flash_id);

          if (emmc_cap > (1024 * 1024 * 1024)) {
            display_printf("EMMC CAP= %d GB\n", (unsigned int)(emmc_cap >> 30));
          } else if (emmc_cap > (1024 * 1024)) {
            display_printf("EMMC CAP= %d MB\n", (unsigned int)(emmc_cap >> 20));
          } else {
            display_printf("EMMC CAP= %d Bytes\n", (unsigned int)emmc_cap);
          }
          display_printf("SE RANDOM:\n");
          for (int i = 0; i < 32; i++) {
            display_printf("%02X ", rand_buffer[i]);
          }
          display_printf("\n");
          if (ble_name_state()) {
            display_printf("BLE NAME = %s\n", ble_get_name());
          }
          if (ble_battery_state()) {
            display_printf("BATTERY= %d%%\n", battery_cap);
          }
          if (ble_switch_state()) {
            if (ble_get_switch()) {
              display_printf("BLE ON,TURN OFF BLE...\n");
              ble_cmd_req(BLE_BT, BLE_BT_OFF);
              hal_delay(5);
            } else {
              display_printf("BLE OFF,TURN ON BLE...\n");
              ble_cmd_req(BLE_BT, BLE_BT_ON);
              hal_delay(5);
            }
          }

          if (fingerprint_detect) {
            display_fp(10, 620, 88, 112, image_data);
            fingerprint_detect = false;
          }

          display_printf("Poll card...\n");
          nfc_pwr_ctl(true);
          HAL_TIM_Base_Stop(&TimHandle);
          while (1) {
            if (nfc_poll_card()) {
              if (nfc_select_aid((uint8_t *)"\xD1\x56\x00\x01\x32\x83\x40\x01",
                                 8)) {
                if (lite_card_data_exchange_test()) {
                  break;
                }
              } else if (nfc_select_aid(
                             (uint8_t *)"\x6f\x6e\x65\x6b\x65\x79\x2e\x62"
                                        "\x61\x63\x6b\x75\x70\x01",
                             14)) {
                if (lite_card_data_exchange_test()) {
                  break;
                }
              }
            }
          }
          nfc_pwr_ctl(false);
          display_printf("Card test passed\n");
          HAL_TIM_Base_Start(&TimHandle);

          if (!_sdram_test()) {
            display_printf("SDRAM test failed\n");
            while (1)
              ;
          }
          display_printf("SDRAM test passed\n");
          break;
        default:
          break;
      }
    }
    // 100ms
    if ((current - previous) > (TIMER_1S / 2)) {
      previous = current;
      emmc_cap = emmc_get_capacity_in_bytes();
      if (emmc_cap == 0) {
        display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2,
                            "EMMC test faild", -1, FONT_NORMAL, COLOR_RED,
                            COLOR_BLACK);
        while (1)
          ;
      }
      if (get_hw_ver() < HW_VER_3P0A) {
        flash_id = qspi_flash_read_id();
        if (flash_id == 0) {
          display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2,
                              "SPI-FLASH test faild", -1, FONT_NORMAL,
                              COLOR_RED, COLOR_BLACK);
          while (1)
            ;
        }
      }

      if (!se_get_rand(rand_buffer, 32)) {
        display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "SE test faild",
                            -1, FONT_NORMAL, COLOR_RED, COLOR_BLACK);
        while (1)
          ;
      }

      if (!ble_name_state()) {
        ble_cmd_req(BLE_VER, BLE_VER_ADV);
        hal_delay(5);
      }
      if (!ble_battery_state()) {
        ble_cmd_req(BLE_PWR, BLE_PWR_EQ);
        hal_delay(5);
      }
      if (!ble_switch_state()) {
        ble_cmd_req(BLE_BT, BLE_BT_STA);
        hal_delay(5);
      }

      fingerprint_detect = false;
      if (FpsDetectFinger() == 1) {
        if (FpsGetImageData(image_data) == 0) {
          fingerprint_detect = true;
        }
      }
    }

    flashled_now = HAL_GetTick();
    if (flashled_value) {
      if (flashled_now - flashled_pre > 1000) {
        flashled_pre = flashled_now;
        flashled_value = 0;
        ble_set_flashled(flashled_value);
      }
    } else {
      if (flashled_now - flashled_pre > 10000) {
        flashled_pre = flashled_now;
        flashled_value = 1;
        ble_set_flashled(flashled_value);
      }
    }

#define FONT_HEIGHT 25

    battery_now = HAL_GetTick();
    if (battery_now - battery_pre > 1000) {
      battery_pre = battery_now;
      uint16_t voltage = 0;
      char battery_str[64] = {0};
      uint16_t battery_info_offset_y = 640;
      ble_get_battery_voltage(&voltage);
      if (voltage_last != voltage) {
        voltage_last = voltage;
        mini_snprintf(battery_str, sizeof(battery_str), "voltage %d mv",
                      (voltage_last));
        display_bar(0, battery_info_offset_y - 20, 360, FONT_HEIGHT,
                    COLOR_BLACK);
        display_text(0, battery_info_offset_y, battery_str, -1, FONT_NORMAL,
                     COLOR_WHITE, COLOR_BLACK);
      }
      battery_info_offset_y += FONT_HEIGHT;
      uint16_t current = 0;
      ble_get_battery_charging_current(&current);
      if (current_last != current) {
        current_last = current;
        mini_snprintf(battery_str, sizeof(battery_str),
                      "charging current %d ma", (current_last));
        display_bar(0, battery_info_offset_y - 20, 360, FONT_HEIGHT,
                    COLOR_BLACK);
        display_text(0, battery_info_offset_y, battery_str, -1, FONT_NORMAL,
                     COLOR_WHITE, COLOR_BLACK);
      }

      battery_info_offset_y += FONT_HEIGHT;

      uint16_t discharging_current = 0;
      ble_get_battery_discharging_current(&discharging_current);
      if (discahrging_current_last != discharging_current) {
        discahrging_current_last = discharging_current;
        mini_snprintf(battery_str, sizeof(battery_str),
                      "discharging current %d ma", (discahrging_current_last));
        display_bar(0, battery_info_offset_y - 20, 360, FONT_HEIGHT,
                    COLOR_BLACK);
        display_text(0, battery_info_offset_y, battery_str, -1, FONT_NORMAL,
                     COLOR_WHITE, COLOR_BLACK);
      }
      battery_info_offset_y += FONT_HEIGHT;
      if (!ble_charging_state()) {
        ble_cmd_req(BLE_PWR, BLE_PWR_CHARGING);
      } else {
        uint8_t charge_type = ble_get_charge_type();
        if (charge_type_last != charge_type) {
          charge_type_last = charge_type;
          display_bar(0, battery_info_offset_y - 20, 360, FONT_HEIGHT,
                      COLOR_BLACK);
          if (CHARGE_BY_USB == charge_type_last) {
            display_text(0, battery_info_offset_y, "charging via usb", -1,
                         FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
          } else if (CHARGE_BY_WIRELESS == charge_type_last) {
            display_text(0, battery_info_offset_y, "charging  via wireless", -1,
                         FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
          } else {
            display_text(0, battery_info_offset_y, "uncharged", -1, FONT_NORMAL,
                         COLOR_WHITE, COLOR_BLACK);
          }
        }
      }
      battery_info_offset_y += FONT_HEIGHT;
      uint16_t battery_temp = 0;
      if (ble_get_battery_inner_temp(&battery_temp)) {
        mini_snprintf(battery_str, sizeof(battery_str), "battery temp %d c",
                      (battery_temp));
        display_bar(0, battery_info_offset_y - 20, 360, FONT_HEIGHT,
                    COLOR_BLACK);
        display_text(0, battery_info_offset_y, battery_str, -1, FONT_NORMAL,
                     COLOR_WHITE, COLOR_BLACK);
      }
    }

    if (index == 3) {
      motor_tick();
      camera_capture_start();
      if (camera_capture_done()) {
        dma2d_copy_buffer((uint32_t *)CAM_BUF_ADDRESS,
                          (uint32_t *)FMC_SDRAM_LTDC_BUFFER_ADDRESS, 80, 300,
                          WIN_W, WIN_H);
      }
    }

    if ((current - start) > 15 * 60 * TIMER_1S) {
      ble_set_flashled(0);
      test_res.flag = TEST_TESTING;
      test_res.time = current;
      f_lseek(&fil, 0);
      f_write(&fil, &test_res, sizeof(test_res), &bw);
      f_sync(&fil);
      restart();
    }

  } while (current < TEST_DURATION);

  test_res.flag = TEST_PASS;
  test_res.time = current;
  ble_cmd_req(BLE_BT, BLE_BT_ON);
  ble_set_flashled(0);
  f_lseek(&fil, 0);
  f_write(&fil, &test_res, sizeof(test_res), &bw);
  f_sync(&fil);
  restart();
}
#endif

#if !PRODUCTION
bool device_backup_otp(bool overwrite) {
  uint32_t processed = 0;
  FlashLockedData otp_data_buffer = {0};
  FlashLockedData *otp_data_buffer_p = &otp_data_buffer;

  // copy current otp
  memcpy(otp_data_buffer_p, flash_otp_data, sizeof(FlashLockedData));

  // backup to emmc
  emmc_fs_file_write("0:otp.bin", 0, otp_data_buffer_p, sizeof(FlashLockedData),
                     &processed, overwrite, false);

  return true;
}
bool device_restore_otp() {
  uint32_t processed = 0;
  FlashLockedData otp_data_buffer = {0};
  FlashLockedData *otp_data_buffer_p = &otp_data_buffer;

  // restore from emmc
  emmc_fs_file_read("0:otp.bin", 0, otp_data_buffer_p, sizeof(FlashLockedData),
                    &processed);

  // change and write back
  ensure(flash_erase(FLASH_SECTOR_OTP_EMULATOR), NULL);
  ensure(flash_unlock_write(), NULL);
  for (size_t j = 0; j < sizeof(FlashLockedData); j += (sizeof(uint32_t) * 8)) {
    ensure(flash_write_words(FLASH_SECTOR_OTP_EMULATOR, j,
                             (uint32_t *)((uint8_t *)otp_data_buffer_p + j)),
           NULL);
  }
  ensure(flash_lock_write(), NULL);

  // refresh
  device_para_init();

  return true;
}
bool device_overwrite_serial(char *dev_serial) {
  // not set, no need to overwrite
  if (device_set_serial(dev_serial)) {
    device_para_init();
    return true;
  }

  // check serial
  if (!is_valid_ascii((uint8_t *)dev_serial, FLASH_OTP_BLOCK_SIZE - 1)) {
    return false;
  }

  // copy current otp
  FlashLockedData otp_data_buffer = {0};
  FlashLockedData *otp_data_buffer_p = &otp_data_buffer;
  memcpy(otp_data_buffer_p, flash_otp_data, sizeof(FlashLockedData));

  // backup to emmc (no overwrite)
  if (!device_backup_otp(false)) return false;

  // change and write back
  memset(otp_data_buffer_p->flash_otp[FLASH_OTP_DEVICE_SERIAL], 0x00,
         FLASH_OTP_BLOCK_SIZE);
  strlcpy((char *)(otp_data_buffer_p->flash_otp[FLASH_OTP_DEVICE_SERIAL]),
          dev_serial, FLASH_OTP_BLOCK_SIZE);

  ensure(flash_erase(FLASH_SECTOR_OTP_EMULATOR), NULL);
  ensure(flash_unlock_write(), NULL);

  for (size_t j = 0; j < sizeof(FlashLockedData); j += (sizeof(uint32_t) * 8)) {
    ensure(flash_write_words(FLASH_SECTOR_OTP_EMULATOR, j,
                             (uint32_t *)((uint8_t *)otp_data_buffer_p + j)),
           NULL);
  }

  ensure(flash_lock_write(), NULL);

  device_para_init();
  return true;
}
#endif