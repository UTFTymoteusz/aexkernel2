#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    class IDTEntry {
        public:
        uint16_t offset_0;

        uint16_t selector;
        uint8_t  ist;
        uint8_t  attributes;

        uint16_t offset_1;
        uint32_t offset_2;

        uint32_t zero;

        IDTEntry& setOffset(size_t offset);
        IDTEntry& setOffset(void* offset);
        IDTEntry& setType(uint8_t type);
        IDTEntry& setSelector(uint8_t selector);
        IDTEntry& setPresent(bool present);
        IDTEntry& setIST(uint8_t ist);
    } __attribute((packed));

    extern IDTEntry init_IDT[256];

    void setup_idt();
    void load_idt(IDTEntry* idt, size_t entry_count);
    void set_idt_ists();
}