  .syntax unified

  .text

  .global reset_handler
  .type reset_handler, STT_FUNC
reset_handler:

  cpsid if
  ldr r0, =0xE000ED08  // r0 = VTOR address
  ldr r1, =_vector_offset  // r1 = FLASH_APP_START
  str r1, [r0]         // assign

  ldr r0, =_estack      // r0 = stack pointer
  msr msp, r0           // set stack pointer
  dsb
  isb

  // setup environment for subsequent stage of code
  ldr r0, =axiram_start // r0 - point to beginning of axiram
  ldr r1, =axiram_end   // r1 - point to byte after the end of axiram
  ldr r2, =0            // r2 - the word-sized value to be written
  bl memset_reg

  ldr r0, =sram_start   // r0 - point to beginning of SRAM
  ldr r1, =sram_end     // r1 - point to byte after the end of SRAM
  ldr r2, =0            // r2 - the word-sized value to be written
  bl memset_reg

  // copy data in from flash
  ldr r0, =data_vma     // dst addr
  ldr r1, =data_lma     // src addr
  ldr r2, =data_size    // size in bytes
  bl memcpy

  // setup the stack protector (see build script "-fstack-protector-all") with an unpredictable value
  bl rng_get
  ldr r1, = __stack_chk_guard
  str r0, [r1]

  // enter the application code
  bl main

  b shutdown_privileged

  .global MemManage_Handler
  .type MemManage_Handler, STT_FUNC
MemManage_Handler:
  ldr r2, =_sstack
  mrs r1, msp
  ldr r0, =_estack
  msr msp, r0
  cmp r1, r2
  IT lt
  bllt MemManage_Handler_SO
  bl MemManage_Handler_MM

  .end
