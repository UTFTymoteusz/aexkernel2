#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"

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
        auto info = (AEX::Sys::CPU::fault_info*) _info;

        AEX::printk(PRINTK_FAIL "%93$%s%97$ Exception (%i) (%91$%i%97$)\n",
                    exception_names[info->int_no], info->int_no, info->err);
        AEX::printk("RIP: 0x%016lx\n", info->rip);


        if (info->int_no == EXC_PAGE_FAULT) {
            size_t cr2, cr3;

            asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");
            asm volatile("mov rax, cr3; mov %0, rax;" : : "m"(cr3) : "memory");

            printk("%91$%s, %s, %s%97$\n", (info->err & 0x04) ? "User" : "Kernel",
                   (info->err & 0x02) ? "Write" : "Read",
                   (info->err & 0x01) ? "Present" : "Not Present");

            printk("CR2: 0x%016lx  CR3: 0x%016lx\n", cr2, cr3);
        }

        printk("Stack trace:\n");
        Debug::stack_trace(2);

        for (volatile size_t i = 0; i < 2534354353; i++)
            ;

        CPU::broadcastPacket(CPU::ipp_type::HALT);
        kpanic("Unrecoverable processor exception occured in CPU %i", CPU::getCurrentCPUID());
    }
}