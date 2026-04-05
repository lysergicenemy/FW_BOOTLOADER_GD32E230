/* Minimal linker script for GD32E230 bootloader */

/* Highest address of RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* Memory layout */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1K
    RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 4K
}

/* Entry point */
ENTRY(Reset_Handler)

/* Sections */
SECTIONS
{
    /* Vector table at start of FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* Program code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* Initialized data (to be copied from FLASH to RAM) */
    _sidata = LOADADDR(.data);
    
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    /* Uninitialized data (BSS) */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.eh_frame*)
        *(.init_array*)
        *(.fini_array*)
        *(.preinit_array*)
    }
}
