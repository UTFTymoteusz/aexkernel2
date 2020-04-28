#include "sys/irq.hpp"

#include "aex/kpanic.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"
#include "aex/rcparray.hpp"

#include "cpu/irq.hpp"
#include "kernel/acpi/acpi.hpp"
#include "sys/apic.hpp"
#include "sys/cpu.hpp"
#include "sys/pic.hpp"
#include "sys/pit.hpp"

#include <new>
#include <stddef.h>
#include <stdint.h>

#define CPUID_EDX_FEAT_APIC 0x100

namespace AEX::Sys::IRQ {
    bool is_APIC_present = false;

    ACPI::MADT*      madt;
    RCPArray<IOAPIC> ioapics;

    IOAPIC* find_ioapic(int irq);
    int     find_redirection(int irq);
    size_t  find_apic_tps();

    void init() {
        printk(PRINTK_INIT "Initializing IRQs\n");

        uint32_t eax, ebx, ecx, edx;

        CPU::cpuid(0x01, &eax, &ebx, &ecx, &edx);

        is_APIC_present = edx & CPUID_EDX_FEAT_APIC;

        if (!is_APIC_present)
            kpanic("This computer is too ancient to run this OS\n");

        pics[0] = PIC(0x20, 0x21);
        pics[1] = PIC(0xA0, 0xA1);

        MASTER_PIC->init(32, false);
        MASTER_PIC->setMask(0b11111111);

        SLAVE_PIC->init(40, true);
        SLAVE_PIC->setMask(0b11111111);

        size_t addr = 0xFEE00000;

        madt = ACPI::find_table<ACPI::MADT*>("APIC", 0);
        if (!madt)
            kpanic("This computer is too ancient to run this OS\n");

        addr = madt->apic_addr;

        auto override =
            madt->findEntry<ACPI::MADT::addr_override_t*>(ACPI::MADT::entry_type::LAPIC_ADDR, 0);
        if (override)
            addr = override->addr;

        for (int i = 0; i < 2137; i++) {
            auto ioapic = madt->findEntry<ACPI::MADT::ioapic_t*>(ACPI::MADT::entry_type::IOAPIC, i);
            if (!ioapic)
                break;

            void* mapped = VMem::kernel_pagemap->map(sizeof(IOAPIC), ioapic->addr, PAGE_WRITE);

            auto _ioapic = new IOAPIC(mapped, ioapic->global_interrupt_base);

            for (int j = 0; j < _ioapic->getIRQAmount(); j++) {
                _ioapic->setDestination(j, 0x00);
                _ioapic->setVector(j, 0x20 + 30);
                _ioapic->setMask(j, true);
                _ioapic->setMode(j, IOAPIC::irq_mode::NORMAL);
            }

            ioapics.addRef(_ioapic);
        }

        APIC::map(addr);

        set_vector(0, 0x20 + 2);
        set_mask(0, true);
        set_destination(0, 0);

        printk(PRINTK_OK "IRQs initialized\n");
    }

    void setup_timer() {
        size_t tps = find_apic_tps();

        APIC::setupTimer(0x20 + 0, tps, true);
    }

    IOAPIC* find_ioapic(int irq) {
        for (int i = 0; i < ioapics.count(); i++) {
            auto ioapic = ioapics.get(i);
            if (!ioapic.isPresent())
                continue;

            if (ioapic->irq_base > irq)
                continue;

            if (ioapic->irq_base + ioapic->getIRQAmount() < irq)
                continue;

            return &*ioapic;
        }

        return nullptr;
    }

    int find_redirection(int irq) {
        for (int i = 0; i < 2137; i++) {
            auto _irq =
                madt->findEntry<ACPI::MADT::int_override_t*>(ACPI::MADT::entry_type::IRQ_SOURCE, i);
            if (!_irq)
                break;

            if (_irq->irq_source != irq)
                continue;

            return _irq->global_interrupt;
        }

        return irq;
    }

    void set_vector(int irq, uint8_t vector) {
        irq = find_redirection(irq);

        auto ioapic = find_ioapic(irq);
        irq -= ioapic->irq_base;

        ioapic->setVector(irq, vector);
    }

    void set_mask(int irq, bool mask) {
        irq = find_redirection(irq);

        auto ioapic = find_ioapic(irq);
        irq -= ioapic->irq_base;

        ioapic->setMask(irq, mask);
    }

    void set_destination(int irq, uint8_t destination) {
        irq = find_redirection(irq);

        auto ioapic = find_ioapic(irq);
        irq -= ioapic->irq_base;

        ioapic->setDestination(irq, destination);
    }

    void set_mode(int irq, uint8_t mode) {
        irq = find_redirection(irq);

        auto ioapic = find_ioapic(irq);
        irq -= ioapic->irq_base;

        ioapic->setMode(irq, mode);
    }

    size_t find_apic_tps() {
        bool ints = CPU::checkInterrupts();

        CPU::nointerrupts();

        set_vector(0, 0x20 + 31);
        set_mask(0, false);
        set_destination(0, 0);

        IRQ::irq_mark = false;

        APIC::setupTimer(0x20 + 0);
        PIT::interruptIn(10);

        CPU::interrupts();

        while (!IRQ::irq_mark)
            CPU::waitForInterrupt();

        uint32_t ticks = -APIC::getTimerCounter() * 100;

        if (!ints)
            CPU::nointerrupts();

        // Now we don't need the PIT anymore, mask it to hell
        set_vector(0, 0x20 + 30);
        set_mask(0, true);
        set_destination(0, 0);

        return ticks;
    }
}