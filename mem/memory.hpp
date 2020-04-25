#pragma once

extern void* KERNEL_VMA;

extern void *_start_text, *_end_text;
extern void *_start_rodata, *_end_rodata;
extern void *_start_data, *_end_data;
extern void *_start_bss, *_end_bss;

/*namespace AEX::Memory {
    extern void *start_text, *end_text;
    extern void *start_rodata, *end_rodata;
    extern void *start_data, *end_data;
    extern void *start_bss, *end_bss;
}*/