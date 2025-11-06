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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/nlr.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "shared/runtime/pyexec.h"

#include "ports/stm32/gccollect.h"
#include "ports/stm32/pendsv.h"

#include "debug_utils.h"
// #include "adc.h"
#include "bl_check.h"
#include "board_capabilities.h"
#include "common.h"
#include "compiler_traits.h"
#include "display.h"
#include "emmc_fs.h"
#include "flash.h"
#include "hardware_version.h"
#include "image.h"
#include "mpu.h"
#include "random_delays.h"
#include "systick.h"
#include "usart.h"
#ifdef SYSTEM_VIEW
#include "systemview.h"
#endif
#include "rng.h"
// #include "sdcard.h"
#include "adc.h"
#include "camera.h"
#include "device.h"
#include "fingerprint.h"
#include "mipi_lcd.h"
#include "motor.h"
#include "nfc.h"
#include "qspi_flash.h"
#include "se_thd89.h"
#include "spi_legacy.h"
#include "supervise.h"
#include "systick.h"
#include "thd89.h"
#include "timer.h"
#include "touch.h"
#ifdef USE_SECP256K1_ZKP
#include "zkp_context.h"
#endif
#include "cm_backtrace.h"
#include "version.h"

// from util.s
extern void shutdown_privileged(void);

static void copyflash2sdram(void) {
  extern int _flash2_load_addr, _flash2_start, _flash2_end;
  volatile uint32_t *dst = (volatile uint32_t *)&_flash2_start;
  volatile uint32_t *end = (volatile uint32_t *)&_flash2_end;
  volatile uint32_t *src = (volatile uint32_t *)&_flash2_load_addr;

  while (dst < end) {
    *dst = *src;
    if (*dst != *src) {
      error_shutdown("Internal error", "(CF2S)", NULL, NULL);
    }
    dst++;
    src++;
  }
}

int main(void) {
  SystemCoreClockUpdate();
  dwt_init();

  mpu_config_boardloader(sectrue, secfalse);
  mpu_config_bootloader(sectrue, secfalse);
  mpu_config_firmware(sectrue, sectrue);
  mpu_config_base();  // base config last as it contains deny access layers and
                      // mpu may already running
  mpu_ctrl(sectrue);  // ensure enabled

  // disable all external communication or user input irq
  // will be re-enabled later by calling their init function
  // bluetooth uart
  HAL_NVIC_DisableIRQ(UART4_IRQn);
  HAL_NVIC_ClearPendingIRQ(UART4_IRQn);
  // bluetooth spi
  HAL_NVIC_DisableIRQ(SPI2_IRQn);
  HAL_NVIC_ClearPendingIRQ(SPI2_IRQn);
  HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
  HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
  // usb
  HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
  HAL_NVIC_ClearPendingIRQ(OTG_HS_IRQn);

  // re-enable global irq
  __enable_irq();
  __enable_fault_irq();

  lcd_ltdc_dsi_disable();
  sdram_reinit();
  // lcd_para_init(DISPLAY_RESX, DISPLAY_RESY, LCD_PIXEL_FORMAT_RGB565);
  lcd_ltdc_dsi_enable();
  lcd_pwm_init();
  touch_init();
  adc_init();
  

  ensure_emmcfs(emmc_fs_init(), "emmc_fs_init");
  ensure_emmcfs(emmc_fs_mount(true, false), "emmc_fs_mount");
  if (get_hw_ver() < HW_VER_3P0A) {
    qspi_flash_init();
    qspi_flash_config();
    qspi_flash_memory_mapped();
  }

  cm_backtrace_init("firmware", hw_ver_to_str(get_hw_ver()), ONEKEY_VERSION);

  ble_usart_init();
  spi_slave_init();

  random_delays_init();
  collect_hw_entropy();

  motor_init();
  thd89_init();
  camera_init();
  fingerprint_init();
  nfc_init();

  timer_init();
  display_clear();
  lcd_add_second_layer();
  pendsv_init();

  device_test(false);
  device_burnin_test(false);

  device_para_init();
  ensure(se_sync_session_key(), "se start up failed");

  uint32_t bootloader_version = get_bootloader_version();

  bootloader_version >>= 8;

  if (bootloader_version >= 0x020503) {
    // bootloader version is greater than 2.5.3, firmware copy is not needed
  } else {
    copyflash2sdram();
  }

#ifdef RDI
  rdi_start();
#endif

#ifdef SYSTEM_VIEW
  enable_systemview();
#endif

#if !PRODUCTION
  // enable BUS fault and USAGE fault handlers
  SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk);
#endif

#ifdef USE_SECP256K1_ZKP
  ensure(sectrue * (zkp_context_init() == 0), NULL);
#endif
  printf("CORE: Preparing stack\n");
  // Stack limit should be less than real stack size, so we have a chance
  // to recover from limit hit.
  mp_stack_set_top(&_estack);
  mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);

#if MICROPY_ENABLE_PYSTACK
  static mp_obj_t pystack[2048];
  mp_pystack_init(pystack, &pystack[MP_ARRAY_SIZE(pystack)]);
#endif

  // GC init
  printf("CORE: Starting GC\n");
  gc_init(&_heap_start, &_heap_end);

  // Interpreter init
  printf("CORE: Starting interpreter\n");
  mp_init();
  mp_obj_list_init(mp_sys_argv, 0);
  mp_obj_list_init(mp_sys_path, 0);
  mp_obj_list_append(
      mp_sys_path,
      MP_OBJ_NEW_QSTR(MP_QSTR_));  // current dir (or base dir of the script)

  // Execute the main script
  printf("CORE: Executing main script\n");
  pyexec_frozen_module("main.py");
  // Clean up
  printf("CORE: Main script finished, cleaning up\n");
  mp_deinit();

  return 0;
}

// MicroPython default exception handler

void __attribute__((noreturn)) nlr_jump_fail(void *val) {
  error_shutdown("Internal error", "(UE)", NULL, NULL);
}

// interrupt handlers

void NMI_Handler(void) {
  // Clock Security System triggered NMI
  // if ((RCC->CIR & RCC_CIR_CSSF) != 0)
  { error_shutdown("Internal error", "(CS)", NULL, NULL); }
}

// Show fault
void ShowHardFault(void) {
  error_shutdown("Internal error", "(HF)", NULL, NULL);
}

void ShowMemManage_MM(void) {
  error_shutdown("Internal error", "(MM)", NULL, NULL);
}

void ShowMemManage_SO(void) {
  error_shutdown("Internal error", "(SO)", NULL, NULL);
}

void ShowBusFault(void) {
  error_shutdown("Internal error", "(BF)", NULL, NULL);
}

void ShowUsageFault(void) {
  error_shutdown("Internal error", "(UF)", NULL, NULL);
}

__attribute__((noreturn)) void reboot_to_bootloader() {
  jump_to_with_flag(BOOTLOADER_START + IMAGE_HEADER_SIZE,
                    BOOT_TARGET_BOOTLOADER);
  for (;;)
    ;
}

void SVC_C_Handler(uint32_t *stack) {
  uint8_t svc_number = ((uint8_t *)stack[6])[-2];
  switch (svc_number) {
    case SVC_ENABLE_IRQ:
      HAL_NVIC_EnableIRQ(stack[0]);
      break;
    case SVC_DISABLE_IRQ:
      HAL_NVIC_DisableIRQ(stack[0]);
      break;
    case SVC_SET_PRIORITY:
      NVIC_SetPriority(stack[0], stack[1]);
      break;
#ifdef SYSTEM_VIEW
    case SVC_GET_DWT_CYCCNT:
      cyccnt_cycles = *DWT_CYCCNT_ADDR;
      break;
#endif
    case SVC_SHUTDOWN:
      shutdown_privileged();
      for (;;)
        ;
      break;
    case SVC_RESET_SYSTEM:
      HAL_NVIC_SystemReset();
      while (1)
        ;
      break;
    default:
      stack[0] = 0xffffffff;
      break;
  }
}

__attribute__((naked)) void SVC_Handler(void) {
  __asm volatile(
      " tst lr, #4    \n"    // Test Bit 3 to see which stack pointer we should
                             // use.
      " ite eq        \n"    // Tell the assembler that the nest 2 instructions
                             // are if-then-else
      " mrseq r0, msp \n"    // Make R0 point to main stack pointer
      " mrsne r0, psp \n"    // Make R0 point to process stack pointer
      " b SVC_C_Handler \n"  // Off to C land
  );
}

// MicroPython builtin stubs

mp_import_stat_t mp_import_stat(const char *path) {
  return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(uint n_args, const mp_obj_t *args, mp_map_t *kwargs) {
  return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
