#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    enum gdt_flags_t : uint8_t {
        GDT_FL_X64         = 0x02,
        GDT_FL_SIZE        = 0x04,
        GDT_FL_GRANULARITY = 0x08,
    };

    enum gdt_access_t : uint8_t {
        GDT_AC_ACCESSED    = 0x01,
        GDT_AC_READ_WRITE  = 0x02,
        GDT_AC_DIR_CONFORM = 0x04,
        GDT_AC_EXECUTABLE  = 0x08,
        GDT_AC_CODE_DATA   = 0x10,
        GDT_AC_RING_0      = 0x00,
        GDT_AC_RING_1      = 0x20,
        GDT_AC_RING_2      = 0x40,
        GDT_AC_RING_3      = 0x60,
        GDT_AC_PRESENT     = 0x80,
    };

    struct gdt_entry {
        uint16_t limit_low;

        uint16_t base_low;
        uint8_t  base_middle;

        uint8_t access;

        uint8_t limit_high : 4;
        uint8_t flags : 4;

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