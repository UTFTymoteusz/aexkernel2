#include "sys/irq/apic.hpp"

#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"

constexpr auto IA32_APIC_BASE_MSR = 0x1B;

namespace AEX::Sys::IRQ {
    void* APIC::addr;

    void APIC::map(Mem::Phys::phys_addr phys) {
        if (addr != nullptr)
            return;

        addr = Mem::kernel_pagemap->map(CPU::PAGE_SIZE, phys, PAGE_WRITE);
    }

    uint32_t APIC::read(int reg) {
        return *((volatile uint32_t*) ((size_t) addr + reg));
    }

    void APIC::write(int reg, uint32_t val) {
        *((volatile uint32_t*) ((size_t) addr + reg)) = val;
    }

    void APIC::init() {
        write(0xF0, 0x1FF);
    }

    int APIC::id() {
        return read(0x20) >> 24;
    }

    void APIC::timer(uint32_t vector) {
        write(0x320, vector);
        write(0x380, -1);
        write(0x3E0, 0x03);
    }

    void APIC::timer(uint32_t vector, uint32_t initial_count, bool periodic) {
        write(0x320, vector | (periodic ? (1 << 17) : 0x00));
        write(0x380, initial_count);
        write(0x3E0, 0x03);
    }

    uint32_t APIC::counter() {
        return read(0x390);
    }

    uint32_t APIC::initial() {
        return read(0x380);
    }

    void APIC::interrupt(uint8_t dst, uint8_t vector) {
        bool ints = CPU::checkInterrupts();

        CPU::nointerrupts();

        write(0x310, dst << 24);
        write(0x300, (1 << 14) | vector);

        volatile size_t counter = 0;

        while (read(0x300) & (1 << 12)) {
            counter++;

            if (counter > 2432432)
                kpanic("apic stuck, apic stuck");
        }

        if (ints)
            CPU::interrupts();
    }

    void APIC::init(uint8_t dst) {
        bool ints = CPU::checkInterrupts();

        CPU::nointerrupts();

        write(0x310, dst << 24);
        write(0x300, (5 << 8));

        while (read(0x300) & (1 << 12))
            ;

        if (ints)
            CPU::interrupts();
    }

    void APIC::sipi(uint8_t dst, uint8_t page) {
        bool ints = CPU::checkInterrupts();

        CPU::nointerrupts();

        write(0x310, dst << 24);
        write(0x300, (6 << 8) | (uint32_t) page);

        while (read(0x300) & (1 << 12))
            ;

        if (ints)
            CPU::interrupts();
    }

    void APIC::nmi(uint8_t dst) {
        bool ints = CPU::checkInterrupts();

        CPU::nointerrupts();

        write(0x310, dst << 24);
        write(0x300, (4 << 8));

        while (read(0x300) & (1 << 12))
            ;

        if (ints)
            CPU::interrupts();
    }

    void APIC::eoi() {
        write(0xB0, 0x00);
    }

    IOAPIC::IOAPIC(void* mapped, int base) {
        irq_base = base;

        addr_reg = (uint32_t*) mapped;
        data_reg = (uint32_t*) ((size_t) mapped + 0x10);
    }

    int IOAPIC::amount() {
        return (read(0x01) >> 16) & 0xFF;
    }

    void IOAPIC::vector(int irq, uint8_t vector) {
        uint32_t val = read(0x10 + irq * 2);

        val &= 0xFFFFFF00;
        val |= vector;

        write(0x10 + irq * 2, val);
    }

    void IOAPIC::mask(int irq, bool mask) {
        uint32_t val = read(0x10 + irq * 2);

        if (mask)
            val |= (1 << 16);
        else
            val &= ~(1 << 16);

        write(0x10 + irq * 2, val);
    }

    void IOAPIC::destination(int irq, uint8_t destination) {
        uint32_t vala = read(0x10 + irq * 2);
        uint32_t valb = read(0x10 + irq * 2 + 1);

        vala &= ~(1 << 11);

        valb &= ~(0xFF000000);
        valb |= destination << 24;

        write(0x10 + irq * 2, vala);
        write(0x10 + irq * 2 + 1, valb);
    }

    void IOAPIC::mode(int irq, uint8_t mode) {
        uint32_t val = read(0x10 + irq * 2);

        val &= ~(0b111 << 8);
        val |= mode << 8;

        write(0x10 + irq * 2, val);
    }

    uint32_t IOAPIC::read(int reg) {
        *addr_reg = reg;
        return *data_reg;
    }

    void IOAPIC::write(int reg, uint32_t val) {
        *addr_reg = reg;
        *data_reg = val;
    }
}