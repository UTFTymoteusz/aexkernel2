#include "sys/mcore.hpp"

#include "kernel/acpi/acpi.hpp"
#include "kernel/string.hpp"
#include "lib/rcparray.hpp"
#include "mem/vmem.hpp"
#include "sys/apic.hpp"

#define PROCESSOR_ENABLED 1 << 0
#define PROCESSOR_ONLINE 1 << 1

#define TRAMPOLINE_ADDR 0x1000 // Must be page aligned!

namespace AEX::Sys::MCore {
    extern "C" char _binary_bin_obj___arch_x64_boot_mcore_asmr_o_start;
    void*           ptr = (void*) &_binary_bin_obj___arch_x64_boot_mcore_asmr_o_start;

    volatile int ap_started_count;

    RCPArray<CPU> CPUs;

    bool start_ap(int id, int apic_id);
    void finalize_ap(int id, void* stack);

    void setup_trampoline(int id) {
        volatile size_t stack = (size_t) VMem::kernel_pagemap->alloc(1024, PAGE_WRITE) + 1024;

        // For some reason g++ offsets some of the addresses by 8
        asm volatile("    \
            sidt [0x510]; \
            sgdt [0x520]; \
            \
            mov qword [0x530 - 8], %0; \
            mov qword [0x540 - 8], %1; \
            mov qword [0x550 - 8], %2; \
            \
            mov rax, cr3; \
            mov qword [0x500 - 8], rax; \
        "
                     :
                     : "r"(finalize_ap), "r"(stack), "r"(id)
                     : "memory");

        void* dst = (void*) TRAMPOLINE_ADDR;

        memcpy(dst, (void*) ptr, CPU::PAGE_SIZE);
    }

    void init() {
        printk(PRINTK_INIT "mcore: Initializing\n");

        auto madt = ACPI::find_table<ACPI::madt_t*>("APIC", 0);
        int  id   = -1;

        if (madt == nullptr) {
            printk(PRINTK_WARN
                   "mcore: No MADT table found, seems like we're on an ancient system\n");
            printk(PRINTK_WARN "mcore: Failed\n");

            return;
        }

        for (size_t i = 0; i < madt->header.length - sizeof(ACPI::madt_t);) {
            auto entry = (ACPI::madt_entry_t*) &(madt->data[i]);

            // printk("entry: %i, len %i\n", entry->type, entry->len);

            switch (entry->type) {
            case ACPI::madt_entry_type::APIC:
                auto cpu = (ACPI::madt_entry_apic_t*) entry;

                id++;

                if (CPU::getCurrentCPU()->apic_id == cpu->apic_id)
                    break;

                if (!(cpu->flags & PROCESSOR_ENABLED) && !(cpu->flags & PROCESSOR_ONLINE)) {
                    printk(PRINTK_WARN "mcore: Found disabled CPU %i with APIC id of %i\n", cpu->id,
                           cpu->apic_id);
                    break;
                }

                printk("mcore: Found CPU %i with APIC id of %i\n", cpu->id, cpu->apic_id);

                bool result = start_ap(cpu->id, cpu->apic_id);
                if (!result)
                    printk(PRINTK_WARN "mcore: Failed to start CPU %i\n", cpu->id);

                break;
            }

            i += entry->len;
        }

        while (ap_started_count != id)
            ;

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

        for (int j = 0; j < 100; j++) {
            for (int k = 0; k < 800; k++)
                CPU::inportb(0x20);

            if (*signal == 0xAA) {
                success = true;
                break;
            }
        }

        return success;
    }

    void finalize_ap(int id, void* stack) {
        static Spinlock lock;

        auto cpu = new CPU(id);
        CPUs.addRef(cpu);

        CPU::interrupts();

        printk(PRINTK_OK "mcore: CPU %i ready\n", CPU::getCurrentCPUID());

        lock.acquire();
        ap_started_count++;
        lock.release();
    }
};