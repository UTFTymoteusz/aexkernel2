ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    KERNEL_VMA = 0xFFFFFFFF80000000;

    . += 1M;

    .bootstrap ALIGN(4K) :
    {
        *(.mboottrampoline)
        
        _start_text = .;
        *(.bootstrap)
    }

    .bootstrap.bss ALIGN(4K) :
    {
        *bootstrap.o(.bss)
        *bootstrap.o(.bootstrap.bss)
    }

    . += KERNEL_VMA;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.text)
        *(.text.*)
        _end_text = . - KERNEL_VMA;
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        _start_rodata = . - KERNEL_VMA;
        *(.rodata)
        *(.rodata.*)
        _end_rodata = . - KERNEL_VMA;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA)
    {
        _start_data = . - KERNEL_VMA;
        *(.data)
        *(.data.*)
        _end_data = . - KERNEL_VMA;
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        _start_bss = . - KERNEL_VMA;
        *(COMMON)
        *(EXCLUDE_FILE("*bootstrap.o") .bss)
        _end_bss = . - KERNEL_VMA;
    }

    .whocares :
    {
        *(.note.gnu.build-id)
        *(.comment)
        *(.stab)
        *(.stabstr)
        *(.eh_frame)
        *(.__GNU_EH_FRAME_HDR)
        *(.debug_loc)
        *(.debug_info)
        *(.debug_aranges)
        *(.debug_ranges)
        *(.debug_abbrev)
        *(.debug_line)
        *(.debug_str)
    }
}