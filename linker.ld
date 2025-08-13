/* linker.ld */
/* This script tells the linker how to arrange the kernel in memory. */

ENTRY(_start) /* The entry point of our kernel. */

SECTIONS
{
    /* Start placing sections at the 1 Megabyte address. */
    . = 1M;

    .text :
    {
        *(.multiboot) /* Place the Multiboot header at the very beginning. */
        *(.text)      /* Place all other code after it. */
    }

    .rodata ALIGN (4K) : {
        *(.rodata)
    }

    .data ALIGN (4K) : {
        *(.data)
    }

    .bss ALIGN (4K) : {
        *(.bss)
    }
}