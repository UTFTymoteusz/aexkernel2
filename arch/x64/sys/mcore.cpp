#include "sys/mcore.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"
#include "aex/sys/acpi.hpp"
#include "aex/sys/acpi/madt.hpp"

#include "cpu/gdt.hpp"
#include "cpu/idt.hpp"
#include "cpu/tss.hpp"
#include "sys/irq.hpp"
#include "sys/irq/apic.hpp"

constexpr auto TRAMPOLINE_ADDR = 0x1000; // Must be page aligned!

namespace AEX::Sys::MCore {
    using madt = ACPI::madt;

    extern "C" char _binary_bin_obj___arch_x64___boot_mcore_asmr_o_start;
    void*           trampoline = (void*) &_binary_bin_obj___arch_x64___boot_mcore_asmr_o_start;

    int   cpu_count = 0;
    CPU** CPUs;

    bool start(int id, int apic_id);
    void finalize(int id, void* stack);
    void ap_wait();

    void setup_trampoline(int id) {
        volatile size_t stack = (size_t) Mem::kernel_pagemap->alloc(4096, PAGE_WRITE) + 4096;

        // For some reason g++ offsets some of the addresses by 8
        asm volatile("    \
            sidt [0x510]; \
            sgdt [0x520]; \
            \
            mov qword [0x530 - 8], %0; \
            mov qword [0x540 - 8], %1; \
            mov qword [0x550 - 8], %2; \
            mov qword [0x560 - 8], %3; \
            \
            mov rax, cr3; \
            mov qword [0x500 - 8], rax; \
        "
                     :
                     : "r"(finalize), "r"(stack), "r"(id), "r"(ap_wait)
                     : "memory");

        void* dst = (void*) TRAMPOLINE_ADDR;

        memcpy(dst, (void*) trampoline, CPU::PAGE_SIZE);
    }

    void init() {
        printk(PRINTK_INIT "mcore: Initializing\n");

        // We can assume it exists because the IRQ phase would panic the kernel otherwise
        auto _madt = (madt*) ACPI::find_table("APIC", 0);
        int  id    = 0;

        // We need to count 'em up so the GDT can be prepared
        for (int i = 0; i <= 2137; i++) {
            auto entry = _madt->findEntry<madt::lapic*>(madt::LAPIC, i);
            if (!entry)
                break;

            if (!entry->canStart())
                continue;

            cpu_count++;
        }

        CPUs = new CPU* [cpu_count] { CPU::current() };
        tss* tsses[cpu_count];

        mcore_gdt(tsses);
        set_idt_ists();

        for (int i = 0; i <= 2137; i++) {
            auto entry = _madt->findEntry<madt::lapic*>(madt::LAPIC, i);
            if (!entry)
                break;

            if (!entry->canStart()) {
                printk(PRINTK_WARN "mcore: Found disabled CPU r:%i with APIC id of %i\n", entry->id,
                       entry->apic_id);
                continue;
            }

            if (entry->apic_id == CPU::current()->apic_id)
                continue;

            id++;

            printk("mcore: Found cpu%i (r:%i) with APIC id of %i\n", id, entry->id, entry->apic_id);

            if (!start(id, entry->apic_id))
                printk(PRINTK_WARN "mcore: Failed to start cpu%i\n", id);
        }

        for (int i = 0; i < 24; i++)
            IRQ::set_destination(i, i % cpu_count);

        for (int i = 0; i < cpu_count; i++) {
            // We need to check because a CPU can fail to start.
            if (!CPUs[i])
                continue;

            CPUs[i]->m_tss = tsses[i];
        }

        printk(PRINTK_OK "mcore: Initialized\n");
    }

    bool start(int id, int apic_id) {
        setup_trampoline(id);

        IRQ::APIC::init(apic_id);

        for (int j = 0; j < 50; j++)
            CPU::inportb(0x20);

        IRQ::APIC::sipi(apic_id, TRAMPOLINE_ADDR / CPU::PAGE_SIZE);

        bool success             = false;
        uint8_t* volatile signal = (uint8_t*) TRAMPOLINE_ADDR;

        for (int j = 0; j < 16000; j++) {
            for (int k = 0; k < 300; k++)
                CPU::inportb(0x20);

            if (*signal == 0xAA) {
                success = true;
                break;
            }
        }

        return success;
    }

    void finalize(int id, void*) {
        IRQ::APIC::init();

        auto cpu = new CPU(id);
        cpu->initLocal();

        CPUs[id] = cpu;

        printk(PRINTK_OK "mcore: cpu%i: Ready\n", CPU::currentID());
    }

    void ap_wait() {
        auto cpu = CPU::current();

        CPU::interrupts();
        cpu->in_interrupt--;

        while (true)
            CPU::waitForInterrupt();
    }
}