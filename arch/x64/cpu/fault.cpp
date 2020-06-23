#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"

#define EXC_DEBUG 1
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
        Sys::CPU::getCurrentCPU()->in_interrupt++;

        auto info = (AEX::Sys::CPU::fault_info*) _info;
        auto cpu  = CPU::getCurrentCPU();

        int  delta = 0;
        auto name  = Debug::symbol_addr2name((void*) info->rip, &delta);
        if (!name)
            name = "no idea";

        printk_fault();

        AEX::printk(PRINTK_FAIL "cpu%i: %93$%s%$ Exception (%i) (%91$%i%$)\n",
                    CPU::getCurrentCPUID(), exception_names[info->int_no], info->int_no, info->err);
        AEX::printk("RIP: 0x%016lx <%s+0x%x>\n", info->rip, name, delta);

        AEX::printk("TID: %8i\n", cpu->current_tid);

        /*if (Proc::threads[cpu->current_tid] && Proc::threads[cpu->current_tid]->parent)
            AEX::printk("PID: %8i, TID: %8i\n", Proc::threads[cpu->current_tid]->parent->pid,
                        cpu->current_tid);
        else
            AEX::printk("PID: %8s, TID: %8s\n", "*idk*", "*idk*");*/

        if (info->int_no == EXC_PAGE_FAULT) {
            size_t cr2, cr3;

            asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");
            asm volatile("mov rax, cr3; mov %0, rax;" : : "m"(cr3) : "memory");

            printk("%91$%s, %s, %s%$\n", (info->err & 0x04) ? "User" : "Kernel",
                   (info->err & 0x02) ? "Write" : "Read",
                   (info->err & 0x01) ? "Present" : "Not Present");

            printk("CR2: 0x%016lx  CR3: 0x%016lx\n", cr2, cr3);
        }

        if (info->int_no == EXC_DEBUG) {
            Debug::stack_trace();

            for (volatile size_t i = 0; i < 84354325; i++)
                ;

            Sys::CPU::getCurrentCPU()->in_interrupt--;
            return;
        }

        printk("RAX: 0x%016lx  RBX: 0x%016lx  RCX: 0x%016lx  RDX: 0x%016lx\n", info->rax, info->rbx,
               info->rcx, info->rdx);
        printk("RSI: 0x%016lx  RDI: 0x%016lx  RSP: 0x%016lx  RBP: 0x%016lx\n", info->rsi, info->rdi,
               info->rsp, info->rbp);

        printk("R8 : 0x%016lx  R9 : 0x%016lx  R10: 0x%016lx  R11: 0x%016lx\n", info->r8, info->r9,
               info->r10, info->r11);
        printk("R12: 0x%016lx  R13: 0x%016lx  R14: 0x%016lx  R15: 0x%016lx\n", info->r12, info->r13,
               info->r14, info->r15);

        printk("RFLAGS: 0x%016lx\n", info->rflags);

        kpanic("Unrecoverable processor exception occured in CPU %i", CPU::getCurrentCPUID());

        Sys::CPU::getCurrentCPU()->in_interrupt--;
    }
}