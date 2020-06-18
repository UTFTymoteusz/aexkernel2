#include "sys/mcore.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "cpu/gdt.hpp"
#include "cpu/idt.hpp"
#include "cpu/tss.hpp"
#include "kernel/acpi/acpi.hpp"
#include "sys/apic.hpp"

#define TRAMPOLINE_ADDR 0x1000 // Must be page aligned!

namespace AEX::Sys::MCore {
    using MADT = AEX::ACPI::MADT;

    extern "C" char _binary_bin_obj___arch_x64___boot_mcore_asmr_o_start;
    void*           trampoline = (void*) &_binary_bin_obj___arch_x64___boot_mcore_asmr_o_start;

    int  cpu_count = 0;
    CPU* CPUs[64];

    bool start_ap(int id, int apic_id);
    void finalize_ap(int id, void* stack);
    void ap_wait();

    void setup_trampoline(int id) {
        volatile size_t stack = (size_t) VMem::kernel_pagemap->alloc(4096, PAGE_WRITE) + 4096;

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
                     : "r"(finalize_ap), "r"(stack), "r"(id), "r"(ap_wait)
                     : "memory");

        void* dst = (void*) TRAMPOLINE_ADDR;

        memcpy(dst, (void*) trampoline, CPU::PAGE_SIZE);
    }

    void init() {
        printk(PRINTK_INIT "mcore: Initializing\n");

        // We need to add ourselves to the array
        CPUs[0] = CPU::getCurrentCPU();

        // We can assume it exists because the IRQ phase would panic the kernel otherwise
        auto madt = (MADT*) ACPI::find_table("APIC", 0);
        int  id   = 0;

        // We need to count 'em up so the GDT can be prepared
        for (int i = 0; i <= 2137; i++) {
            auto entry = madt->findEntry<MADT::lapic*>(MADT::entry_type::LAPIC, i);
            if (!entry)
                break;

            if (!entry->canStart())
                continue;

            cpu_count++;
        }

        init_gdt();
        set_idt_ists();

        for (int i = 0; i <= 2137; i++) {
            auto entry = madt->findEntry<MADT::lapic*>(MADT::entry_type::LAPIC, i);
            if (!entry)
                break;

            if (!entry->canStart()) {
                printk(PRINTK_WARN "mcore: Found disabled CPU r:%i with APIC id of %i\n", entry->id,
                       entry->apic_id);
                continue;
            }

            if (entry->apic_id == CPU::getCurrentCPU()->apic_id)
                continue;

            id++;

            printk("mcore: Found cpu%i (r:%i) with APIC id of %i\n", id, entry->id, entry->apic_id);

            if (!start_ap(id, entry->apic_id))
                printk(PRINTK_WARN "mcore: Failed to start cpu%i\n", id);
        }

        printk(PRINTK_OK "mcore: Initialized\n");
    }

    bool start_ap(int id, int apic_id) {
        setup_trampoline(id);

        APIC::sendINIT(apic_id);

        for (int j = 0; j < 50; j++)
            CPU::inportb(0x20);

        APIC::sendSIPI(apic_id, TRAMPOLINE_ADDR / CPU::PAGE_SIZE);

        bool              success = false;
        volatile uint8_t* signal  = (uint8_t*) TRAMPOLINE_ADDR;

        for (int j = 0; j < 2000; j++) {
            for (int k = 0; k < 2400; k++)
                CPU::inportb(0x20);

            if (*signal == 0xAA) {
                success = true;
                break;
            }
        }

        return success;
    }

    void finalize_ap(int id, void*) {
        APIC::init();

        auto cpu = new CPU(id);
        cpu->initLocal();

        CPUs[id] = cpu;

        CPU::interrupts();

        printk(PRINTK_OK "mcore: cpu%i: Ready\n", CPU::getCurrentCPUID());
    }

    void ap_wait() {
        while (true)
            CPU::waitForInterrupt();
    }
}