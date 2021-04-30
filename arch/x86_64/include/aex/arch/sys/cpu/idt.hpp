#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    struct API idt_entry {
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
    } PACKED;

    API extern idt_entry idt[256];

    API void setup_idt();
    API void load_idt(idt_entry* idt, size_t entry_count);
    API void set_idt_ists();
}