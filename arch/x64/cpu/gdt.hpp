#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    struct gdt_entry {
        enum flags_t : uint8_t {
            X64         = 0x02,
            SIZE        = 0x04,
            GRANULARITY = 0x08,
        };

        enum access_t : uint8_t {
            ACCESSED    = 0x01,
            READ_WRITE  = 0x02,
            DIR_CONFORM = 0x04,
            EXECUTABLE  = 0x08,
            CODE_DATA   = 0x10,
            RING_0      = 0x00,
            RING_1      = 0x20,
            RING_2      = 0x40,
            RING_3      = 0x60,
            PRESENT     = 0x80,
        };

        uint16_t limit_low;

        uint16_t base_low;
        uint8_t  base_middle;

        access_t access;

        uint8_t limit_high : 4;
        flags_t flags : 4;

        uint8_t base_high;

        void setBase(uint32_t base) {
            this->base_low    = base & 0xFFFF;
            this->base_middle = (base >> 16) & 0xFF;
            this->base_high   = (base >> 24) & 0xFF;
        }

        void setLimit(uint32_t limit) {
            this->limit_low  = limit & 0xFFFF;
            this->limit_high = (limit >> 16) & 0x0F;
        }
    } __attribute__((packed));

    void load_gdt(gdt_entry* gdt, int entry_count);

    void init_gdt();
}