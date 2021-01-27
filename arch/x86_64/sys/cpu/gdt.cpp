#include "sys/cpu/gdt.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"

#include "sys/cpu/tss.hpp"
#include "sys/mcore.hpp"

namespace AEX::Sys {
    gdt_entry* gdt;

    void load_gdt(gdt_entry* gdt, int entry_count) {
        uint8_t gdt_descriptor[10];

        uint16_t* size = (uint16_t*) &gdt_descriptor[0];
        uint64_t* addr = (uint64_t*) &gdt_descriptor[2];

        *size = entry_count * sizeof(gdt_entry) - 1;
        *addr = (uint64_t) gdt;

        asm volatile("lgdt [%0]" : : "r"(&gdt_descriptor) : "memory");
    }

    void mcore_gdt(tss** tsses) {
        gdt = new gdt_entry[5 + MCore::cpu_count * 2];

        gdt[0].setBase(0).setLimit(0);
        gdt[0].access = AC_NONE;
        gdt[0].flags  = FL_NONE;

        gdt[1].setBase(0).setLimit(0xFFFFF);
        gdt[1].access = AC_RING_0 | AC_EXECUTABLE | AC_READ_WRITE | AC_CODE_DATA | AC_PRESENT;
        gdt[1].flags  = FL_GRANULARITY | FL_X64;

        gdt[2].setBase(0).setLimit(0xFFFFF);
        gdt[2].access = AC_RING_0 | AC_READ_WRITE | AC_CODE_DATA | AC_PRESENT;
        gdt[2].flags  = FL_NONE;

        gdt[3].setBase(0).setLimit(0xFFFFF);
        gdt[3].access = AC_RING_3 | AC_READ_WRITE | AC_CODE_DATA | AC_PRESENT;
        gdt[3].flags  = FL_NONE;

        gdt[4].setBase(0).setLimit(0xFFFFF);
        gdt[4].access = AC_RING_3 | AC_EXECUTABLE | AC_READ_WRITE | AC_CODE_DATA | AC_PRESENT;
        gdt[4].flags  = FL_GRANULARITY | FL_X64;

        for (int i = 0; i < MCore::cpu_count * 2; i += 2) {
            auto     m_tss    = new tss();
            uint64_t tss_addr = (uint64_t) m_tss;

            gdt[5 + i].setLimit(sizeof(tss)).setBase(tss_addr);
            gdt[5 + i].access = AC_RING_3 | AC_EXECUTABLE | AC_ACCESSED | AC_PRESENT;
            gdt[5 + i].flags  = FL_GRANULARITY | FL_X64;

            *((uint32_t*) &gdt[5 + i + 1]) = tss_addr >> 32;

            m_tss->ist1 = (size_t) 0;
            m_tss->ist2 = (size_t) Mem::kernel_pagemap->alloc(16384) + 16384;

            tsses[i / 2] = m_tss;
        }

        load_gdt(gdt, 5 + MCore::cpu_count * 2);

        // We need to flush the TSS now
        asm volatile("mov eax, %0; ltr ax;" : : "r"(0x28 + CPU::currentID() * 0x10));
    }
}