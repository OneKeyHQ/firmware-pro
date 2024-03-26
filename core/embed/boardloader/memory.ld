/* Trezor v2 boardloader linker script */

ENTRY(reset_handler)

MEMORY {
  FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
  SRAM   (wal) : ORIGIN = 0x20000000, LENGTH = 128K
  AXIRAM (wal) : ORIGIN = 0x24000000, LENGTH = 512K
}

main_stack_base = ORIGIN(SRAM) + LENGTH(SRAM); /* 8-byte aligned full descending stack */

/* used by the startup code to populate variables used by the C code */
data_lma = LOADADDR(.data);
data_vma = ADDR(.data);
data_size = SIZEOF(.data);

/* used by the startup code to wipe memory */
axiram_start = ORIGIN(AXIRAM);
axiram_end = ORIGIN(AXIRAM) + LENGTH(AXIRAM);

/* used by the startup code to wipe memory */
sram_start = ORIGIN(SRAM);
sram_end = ORIGIN(SRAM) + LENGTH(SRAM);

_codelen = SIZEOF(.vector_table) + SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data);

SECTIONS {
  .vector_table : ALIGN(512) {
    KEEP(*(.vector_table));
  } >FLASH AT>FLASH

  .text : ALIGN(4) {
    *(.text*);
    . = ALIGN(4); /* make the section size a multiple of the word size */
  } >FLASH AT>FLASH

  .rodata : ALIGN(4) {
    *(.rodata*);
    . = ALIGN(4); /* make the section size a multiple of the word size */
  } >FLASH AT>FLASH

  .data : ALIGN(4) {
    *(.data*);
    . = ALIGN(4); /* make the section size a multiple of the word size */
  } >SRAM AT>FLASH

  
  .version_section :
  {
    ASSERT (_codelen <= LENGTH(FLASH) - 32, "Error: Code exceeds memory space!");
    . = ORIGIN(FLASH) + LENGTH(FLASH) - 32;
    KEEP(*(.version_section));
  } > FLASH

  .bss : ALIGN(4) {
    *(.bss*);
    . = ALIGN(4); /* make the section size a multiple of the word size */
  } >SRAM


  .stack : ALIGN(8) {
    . = 4K; /* this acts as a build time assertion that at least this much memory is available for stack use */
  } >SRAM

  .heap : ALIGN(4) {
    
    PROVIDE ( end = . ); /* this is needed, otherwise will have "undefined reference to `end'" error */
    PROVIDE ( _end = . ); /* this is needed, otherwise will have "undefined reference to `end'" error */

    . = 32K; /* this acts as a build time assertion that at least this much memory is available for heap use */
    /*. = ABSOLUTE(sram_end - 16K);  this explicitly sets the end of the heap effectively giving the stack at most 16K */
  } >SRAM
}
