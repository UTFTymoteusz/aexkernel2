#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"

#include "sys/irq/apic.hpp"

namespace AEX::Sys::IRQ {
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