#include "cpu/gdt.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"

#include "cpu/tss.hpp"
#include "sys/mcore.hpp"

namespace AEX::Sys {
    void load_gdt(gdt_entry* gdt, int entry_count) {
        uint8_t gdt_descriptor[10];

        uint16_t* size = (uint16_t*) &gdt_descriptor[0];
        uint64_t* addr = (uint64_t*) &gdt_descriptor[2];

        *size = entry_count * sizeof(gdt_entry) - 1;
        *addr = (uint64_t) gdt;

        asm volatile("lgdt [%0]" : : "r"(&gdt_descriptor) : "memory");
    }

    void init_gdt() {
        auto gdt = new gdt_entry[5 + MCore::cpu_count * 2];

        gdt[0].setBase(0);
        gdt[0].setLimit(0);

        gdt[1].setBase(0);
        gdt[1].setLimit(0xFFFFF);
        gdt[1].access = GDT_AC_RING_0 | GDT_AC_EXECUTABLE | GDT_AC_READ_WRITE | GDT_AC_CODE_DATA |
                        GDT_AC_PRESENT;
        gdt[1].flags = GDT_FL_GRANULARITY | GDT_FL_X64;

        gdt[2].setBase(0);
        gdt[2].setLimit(0xFFFFF);
        gdt[2].access = GDT_AC_RING_0 | GDT_AC_READ_WRITE | GDT_AC_CODE_DATA | GDT_AC_PRESENT;
        gdt[2].flags  = 0;

        gdt[3].setBase(0);
        gdt[3].setLimit(0xFFFFF);
        gdt[3].access = GDT_AC_RING_3 | GDT_AC_READ_WRITE | GDT_AC_CODE_DATA | GDT_AC_PRESENT;
        gdt[3].flags  = 0;

        gdt[4].setBase(0);
        gdt[4].setLimit(0xFFFFF);
        gdt[4].access = GDT_AC_RING_3 | GDT_AC_EXECUTABLE | GDT_AC_READ_WRITE | GDT_AC_CODE_DATA |
                        GDT_AC_PRESENT;
        gdt[4].flags = GDT_FL_GRANULARITY | GDT_FL_X64;

        for (int i = 0; i < MCore::cpu_count * 2; i += 2) {
            auto     _tss     = new tss();
            uint64_t tss_addr = (uint64_t) _tss;

            gdt[5 + i].setLimit(sizeof(tss));
            gdt[5 + i].setBase(tss_addr);
            gdt[5 + i].access =
                GDT_AC_RING_3 | GDT_AC_EXECUTABLE | GDT_AC_ACCESSED | GDT_AC_PRESENT;
            gdt[5 + i].flags = GDT_FL_GRANULARITY | GDT_FL_X64;

            *((uint32_t*) &gdt[5 + i + 1]) = tss_addr >> 32;

            _tss->ist1 = (size_t) Mem::kernel_pagemap->alloc(8192) + 8192;
            _tss->ist2 = (size_t) Mem::kernel_pagemap->alloc(8192) + 8192;
        }

        load_gdt(gdt, 5 + MCore::cpu_count * 2);

        // We need to flush the TSS now
        asm volatile("mov eax, %0; ltr ax;" : : "r"(0x28 + CPU::getCurrentCPUID() * 0x10));
    }
}