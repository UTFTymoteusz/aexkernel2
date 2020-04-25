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

        void setOffset(size_t offset);
        void setType(uint8_t type);
        void setSelector(uint8_t selector);
        void setPresent(bool present);
    } __attribute((packed));

    void load_idt(IDTEntry* idt, size_t entry_count);
}