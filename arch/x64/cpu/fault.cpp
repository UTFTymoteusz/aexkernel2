#include "kernel/kpanic.hpp"
#include "kernel/printk.hpp"
#include "sys/cpu.hpp"

#include <stddef.h>

#define EXC_PAGE_FAULT 14

extern "C" void common_fault_handler(void* info);

namespace AEX::Sys {
    char exception_names[][32] = {
        "Divide by Zero",
        "Debug",
        "NMI",
        "Breakpoint",
        "Overflow",
        "Bound Range Exceeded",
        "Invalid Opcode",
        "Device Not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment Not Present",
        "Stack Segment Fault",
        "General Protection Fault",
        "Page Fault",
        "Reserved",
        "Coprocessor",
        "Alignment Check"
        "Machine Check",
        "SIMD Floating-Point",
        "Virtualization",
    };

    extern "C" void common_fault_handler(void* _info) {
        auto info = (AEX::Sys::CPU::fault_info_t*) _info;

        AEX::printk("%93$%s%97$ Exception (%i) (%91$%i%97$)\n", exception_names[info->int_no],
                    info->int_no, info->err);
        AEX::printk("RIP: 0x%016lx\n", info->rip);

        if (info->int_no == EXC_PAGE_FAULT) {
            size_t cr2, cr3;

            asm volatile("mov %0, cr2;" : : "r"(cr2) : "memory");
            asm volatile("mov %0, cr3;" : : "r"(cr3) : "memory");

            printk("CR2: 0x%016lx  CR3: 0x%016lx\n", cr2, cr3);
        }

        CPU::broadcastPacket(CPU::ipp_type::HALT);
        kpanic("Unrecoverable processor exception occured in CPU %i\n", CPU::getCurrentCPUID());
    }
}