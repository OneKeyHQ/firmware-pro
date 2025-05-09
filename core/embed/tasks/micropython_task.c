#include "micropython_task.h"
#include "cmsis_os2.h"
#include "display.h"
#include "stdio.h"
#include "usart.h"

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
#include "cmsis_os2.h"
#include "test_task.h"
#include "micropython_task.h"
#include "version.h"

static void MicroPythonTask(void *pvParameter);

osThreadId_t g_microPythonTasknId;

void CreateMicroPythonTask(void) {
  const osThreadAttr_t micropythonTaskAttributes = {
      .name = "MicroPythonTask",
      .priority = osPriorityBelowNormal,
      .stack_size = 1024 * 24,
  };
  g_microPythonTasknId = osThreadNew(MicroPythonTask, NULL, &micropythonTaskAttributes);
  if (g_microPythonTasknId == NULL) {
    printf("Failed to create MicroPythonTask\n");
  }
}

static void MicroPythonTask(void *pvParameter) {
  uint32_t count = 0;

  //mp_stack_set_top(&_estack);
  //mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);
  uint32_t sp;
  __asm volatile("mov %0, sp" : "=r"(sp));
  mp_stack_set_top((void *)sp);

  mp_stack_set_limit(24 * 1024 - 1024);

 #if MICROPY_ENABLE_PYSTACK
  static mp_obj_t pystack[2048];
  mp_pystack_init(pystack, &pystack[MP_ARRAY_SIZE(pystack)]);
 #endif

  // GC init
  printf("CORE: Starting GC\n");
  gc_init((void *)MICROPYTHON_HEAP_ADDRESS, (void *)(MICROPYTHON_HEAP_ADDRESS + MICROPYTHON_HEAP_LEN));

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

  while (1) {
    printf("MicroPythonTask running,count=%lu\n", count);
    count++;
    osDelay(100);
  }
}
