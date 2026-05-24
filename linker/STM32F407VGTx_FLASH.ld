ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
  CCMRAM(rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  .text :
  {
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    KEEP(*(.init))
    KEEP(*(.fini))
    _etext = .;
  } > FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    _sdata = .;
    *(.data)
    *(.data*)
    _edata = .;
  } > RAM AT > FLASH

  .bss :
  {
    _sbss = .;
    *(.bss)
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM

  . = ALIGN(8);
  _end = .;

  /DISCARD/ :
  {
    libc.a (*)
    libm.a (*)
    libgcc.a (*)
  }
}