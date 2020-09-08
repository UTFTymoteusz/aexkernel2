#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    struct idt_entry {
        uint16_t offset_0;

        uint16_t selector;
        uint8_t  ist;
        uint8_t  attributes;

        uint16_t offset_1;
        uint32_t offset_2;

        uint32_t zero;

        idt_entry& setOffset(size_t offset);
        idt_entry& setOffset(void* offset);
        idt_entry& setType(uint8_t type);
        idt_entry& setSelector(uint8_t selector);
        idt_entry& setPresent(bool present);
        idt_entry& setIST(uint8_t ist);
    } __attribute((packed));

    extern idt_entry init_IDT[256];

    void setup_idt();
    void load_idt(idt_entry* idt, size_t entry_count);
    void set_idt_ists();
}