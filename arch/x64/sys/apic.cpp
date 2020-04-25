#include "sys/apic.hpp"

#include "kernel/printk.hpp"
#include "mem/pmem.hpp"
#include "mem/vmem.hpp"

#include <stdint.h>

namespace AEX::Sys {
    void* APIC::addr;

    void APIC::map(PMem::phys_addr phys) {
        if (addr != nullptr)
            return;

        addr = VMem::kernel_pagemap->map(CPU::PAGE_SIZE, phys, PAGE_WRITE);
    }

    void APIC::sendInterrupt(uint8_t dst, uint8_t vector) {
        CPU::nointerrupts();

        write(0x310, dst << 24);
        write(0x300, (1 << 14) | vector);

        while (read(0x300) & (1 << 12))
            ;

        CPU::interrupts();
    }

    void APIC::sendINIT(uint8_t dst) {
        CPU::nointerrupts();

        write(0x310, dst << 24);
        write(0x300, (5 << 8));

        while (read(0x300) & (1 << 12))
            ;

        CPU::interrupts();
    }

    void APIC::sendSIPI(uint8_t dst, uint8_t page) {
        CPU::nointerrupts();

        write(0x310, dst << 24);
        write(0x300, (6 << 8) | (uint32_t) page);

        while (read(0x300) & (1 << 12))
            ;

        CPU::interrupts();
    }

    void APIC::eoi() { write(0xB0, 0x00); }

    uint32_t APIC::read(int reg) { return *((uint32_t*) ((size_t) addr + reg)); }
    void     APIC::write(int reg, uint32_t val) { *((uint32_t*) ((size_t) addr + reg)) = val; }


    void* IOAPIC::addr;

    uint32_t* IOAPIC::addressreg;
    uint32_t* IOAPIC::datareg;

    IOAPIC::INTEntry IOAPIC::entries[24];

    void IOAPIC::map(PMem::phys_addr phys) {
        addr = VMem::kernel_pagemap->map(CPU::PAGE_SIZE, phys, PAGE_WRITE);

        addressreg = (uint32_t*) addr;
        datareg    = (uint32_t*) ((size_t) addr + 0x10);
    }

    uint32_t IOAPIC::read(int reg) {
        *addressreg = reg;
        return *datareg;
    }

    void IOAPIC::write(int reg, uint32_t val) {
        *addressreg = reg;
        *datareg    = val;
    }

    uint8_t IOAPIC::INTEntry::getVector() { return IOAPIC::read(0x10 + id * 2); }

    void IOAPIC::INTEntry::setVector(uint8_t vector) {
        uint32_t val = IOAPIC::read(0x10 + id * 2);

        val &= 0xFFFFFF00;
        val |= vector;

        IOAPIC::write(0x10 + id * 2, val);
    }
}