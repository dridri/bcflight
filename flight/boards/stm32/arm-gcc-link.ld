/*
*****************************************************************************
**

**  File        : LinkerScript.ld
**
**  Abstract    : Linker script for STM32F417IGHx Device with
**                1024KByte FLASH, 128KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
**  (c)Copyright Ac6.
**  You may use this file as-is or modify it according to the needs of your
**  project. Distribution of this file (unmodified or modified) is not
**  permitted. Ac6 permit registered System Workbench for MCU users the
**  rights to distribute the assembled, compiled & linked contents of this
**  file as part of an application binary file, provided that it is built
**  using the System Workbench for MCU toolchain.
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = 0x20020000;    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
/*_Min_Heap_Size = 0x1f000;  0x200 */
_Min_Heap_Size = 0x1b000;      /* required amount of heap  */
_Min_Stack_Size = 8192; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
FLASH (rx)      : ORIGIN = 0x8000000, LENGTH = 1024K
RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
CCMRAM (rw)      : ORIGIN = 0x10000000, LENGTH = 64K
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

	. = ALIGN(4);
	KEEP(*(.init));
	. = ALIGN(4);
	__preinit_array_start = .;
	KEEP(*(.preinit_array));
	. = ALIGN(4);
	__preinit_array_end = .;
	__init_array_start = .;
	KEEP(*(SORT(.init_array.*)));
	. = ALIGN(4);
	KEEP(*(.init_array));
	. = ALIGN(4);
	__init_array_end = .;
	KEEP(*(.fini));
	. = ALIGN(4);
	__fini_array_start = .;
	KEEP(*(.fini_array));
	. = ALIGN(4);
	KEEP(*(SORT(.fini_array.*)));
	. = ALIGN(4);
	__fini_array_end = .;

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data : 
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH

  _siccmram = LOADADDR(.ccmram);

  /* CCM-RAM section 
  * 
  * IMPORTANT NOTE! 
  * If initialized variables will be placed in this section,
  * the startup code needs to be modified to copy the init-values.  
  */
  .ccmram :
  {
    . = ALIGN(4);
    _sccmram = .;       /* create a global symbol at ccmram start */
    *(.ccmram)
    *(.ccmram*)
    
    . = ALIGN(4);
    _eccmram = .;       /* create a global symbol at ccmram end */
  } >CCMRAM AT> FLASH

	/* Uninitialized data section */
	. = ALIGN(4);
	.bss :
	{
		_sbss = .;         /* define a global symbol at bss start */
		__bss_start__ = .;
		PROVIDE(__bss_start__ = __bss_start__);

		. = ALIGN(4);
		*(.bss .bss.* .gnu.linkonce.b.*)
		. = ALIGN(4);
		*(COMMON);

		. = ALIGN(4);
		_ebss = .;         /* define a global symbol at bss end */
		__bss_end__ = .;
		PROVIDE(__bss_end__ = __bss_end__);
	} > RAM AT > RAM


	.stack :
	{
		. = ALIGN(8);
		__stack_start = .;
		PROVIDE(__stack_start = __stack_start);

		. = ALIGN(8);
		__main_stack_start = .;
		PROVIDE(__main_stack_start = __main_stack_start);

		. += _Min_Stack_Size;

		. = ALIGN(8);
		__main_stack_end = .;
		PROVIDE(__main_stack_end = __main_stack_end);

		. = ALIGN(8);
		__process_stack_start = .;
		PROVIDE(__process_stack_start = __process_stack_start);

		. += _Min_Stack_Size;

		. = ALIGN(8);
		__process_stack_end = .;
		PROVIDE(__process_stack_end = __process_stack_end);

		. = ALIGN(8);
		__stack_end = .;
		PROVIDE(__stack_end = __stack_end);
	} > RAM AT > RAM

	._user_heap_stack :
	{
		. = ALIGN(4);
		__heap_start = .;
		PROVIDE(__heap_start = __heap_start);

		. += _Min_Heap_Size;

		. = ALIGN(4);
		__heap_end = .;
		PROVIDE(__heap_end = __heap_end);
	} > RAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}


