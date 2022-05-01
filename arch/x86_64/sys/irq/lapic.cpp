#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"

#include "sys/irq/apic.hpp"

namespace AEX::Sys::IRQ {
    void* APIC::addr;

    void APIC::map(Mem::phys_t phys) {
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
        interruptible(false) {
            ASSERT(dst != APIC::id());

            write(0x310, dst << 24);
            write(0x300, (1 << 15) | (1 << 14) | vector);

            while (read(0x300) & (1 << 12))
                ;
        }
    }

    void APIC::init(uint8_t dst) {
        interruptible(false) {
            write(0x310, dst << 24);
            write(0x300, (5 << 8));

            while (read(0x300) & (1 << 12))
                ;
        }
    }

    void APIC::sipi(uint8_t dst, uint8_t page) {
        interruptible(false) {
            write(0x310, dst << 24);
            write(0x300, (6 << 8) | (uint32_t) page);

            while (read(0x300) & (1 << 12))
                ;
        }
    }

    void APIC::nmi(uint8_t dst) {
        interruptible(false) {
            write(0x310, dst << 24);
            write(0x300, (1 << 14) | (4 << 8));

            while (read(0x300) & (1 << 12))
                ;
        }
    }

    void APIC::eoi() {
        write(0xB0, 0x00);
    }
}