ENTRY(_start)

SECTIONS
{
    /* Load the kernel at 16MB to avoid conflicts with UEFI reserved memory.
     * The bootloader uses AllocatePages(AllocateAddress, ...) to place
     * each PT_LOAD segment at its physical address. */
    . = 0x1000000;

    .text : ALIGN(0x1000)
    {
        *(.text .text.*)
    }

    .rodata : ALIGN(0x1000)
    {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(0x1000)
    {
        *(.data .data.*)
    }

    .bss : ALIGN(0x1000)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        __bss_end = .;
    }

    /* End of kernel image — used for placing the backbuffer safely after */
    _kernel_end = .;
}
