#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    struct tss;

    enum gdt_flags_t : uint8_t {
        FL_NONE        = 0x00,
        FL_X64         = 0x02,
        FL_SIZE        = 0x04,
        FL_GRANULARITY = 0x08,
    };

    enum gdt_access_t : uint8_t {
        AC_NONE        = 0x00,
        AC_ACCESSED    = 0x01,
        AC_READ_WRITE  = 0x02,
        AC_DIR_CONFORM = 0x04,
        AC_EXECUTABLE  = 0x08,
        AC_CODE_DATA   = 0x10,
        AC_RING_0      = 0x00,
        AC_RING_1      = 0x20,
        AC_RING_2      = 0x40,
        AC_RING_3      = 0x60,
        AC_PRESENT     = 0x80,
    };

    struct gdt_entry {
        uint16_t limit_low;

        uint16_t base_low;
        uint8_t  base_middle;

        uint8_t access;

        uint8_t limit_high : 4;
        uint8_t flags : 4;

        uint8_t base_high;

        gdt_entry& setBase(uint32_t base) {
            this->base_low    = base & 0xFFFF;
            this->base_middle = (base >> 16) & 0xFF;
            this->base_high   = (base >> 24) & 0xFF;

            return *this;
        }

        gdt_entry& setLimit(uint32_t limit) {
            this->limit_low  = limit & 0xFFFF;
            this->limit_high = (limit >> 16) & 0x0F;

            return *this;
        }
    } __attribute__((packed));

    void load_gdt(gdt_entry* gdt, int entry_count);

    void mcore_gdt(tss** tsses);
}