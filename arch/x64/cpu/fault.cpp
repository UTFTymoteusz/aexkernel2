#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

constexpr auto EXC_DEBUG      = 1;
constexpr auto EXC_NMI        = 2;
constexpr auto EXC_PAGE_FAULT = 14;

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
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Security",
        "Reserved",
    };

    void handle_page_fault(CPU::fault_info* info, CPU* cpu, Proc::Thread* thread);

    inline Proc::Thread::state out(Proc::Thread* thread, CPU*& cpu) {
        auto state = thread->saveState();

        thread->setBusy(1);
        thread->setCritical(0);
        thread->setStatus(Proc::THREAD_RUNNABLE);

        cpu->in_interrupt--;
        CPU::interrupts();

        return state;
    }

    inline void in(Proc::Thread::state& state, Proc::Thread* thread, CPU*& cpu) {
        CPU::nointerrupts();
        cpu = CPU::getCurrent();
        cpu->in_interrupt++;

        thread->loadState(state);
    }

    extern "C" void common_fault_handler(void* _info) {
        CPU::getCurrent()->in_interrupt++;

        auto info   = (AEX::Sys::CPU::fault_info*) _info;
        auto cpu    = CPU::getCurrent();
        auto thread = cpu->currentThread;

        switch (info->int_no) {
        case EXC_PAGE_FAULT:
            handle_page_fault(info, cpu, thread);
            CPU::getCurrent()->in_interrupt--;
            return;
        default:
            break;
        }

        int  delta = 0;
        auto name  = Debug::symbol_addr2name((void*) info->rip, &delta);
        if (!name)
            name = "no idea";

        AEX::printk(PRINTK_FAIL "cpu%i: %93$%s%$ Exception (%i) (%91$%i%$)\n", CPU::getCurrentID(),
                    exception_names[info->int_no], info->int_no, info->err);
        AEX::printk("RIP: 0x%016lx <%s+0x%x>\n", info->rip, name, delta);

        AEX::printk("TID: %8i (b%i, c%i, i%i)\n", cpu->current_tid, thread->_busy,
                    thread->_critical, cpu->in_interrupt);

        printk("RAX: 0x%016lx  RBX: 0x%016lx  RCX: 0x%016lx  RDX: 0x%016lx\n", info->rax, info->rbx,
               info->rcx, info->rdx);
        printk("RSI: 0x%016lx  RDI: 0x%016lx  RSP: 0x%016lx  RBP: 0x%016lx\n", info->rsi, info->rdi,
               info->rsp, info->rbp);

        printk("R8 : 0x%016lx  R9 : 0x%016lx  R10: 0x%016lx  R11: 0x%016lx\n", info->r8, info->r9,
               info->r10, info->r11);
        printk("R12: 0x%016lx  R13: 0x%016lx  R14: 0x%016lx  R15: 0x%016lx\n", info->r12, info->r13,
               info->r14, info->r15);

        printk("RFLAGS: 0x%016lx\n", info->rflags);

        if (info->int_no == EXC_DEBUG) {
            Debug::stack_trace();

            static int counter = 0;

            printk("counter: %i\n", counter);

            counter++;

            for (volatile size_t i = 0; i < 84354325; i++)
                ;

            CPU::getCurrent()->in_interrupt--;
            return;
        }

        if (info->int_no == EXC_NMI) {
            Debug::stack_trace();

            CPU::getCurrent()->in_interrupt--;
            return;
        }

        kpanic("Unrecoverable processor exception occured in CPU %i", CPU::getCurrentID());

        CPU::getCurrent()->in_interrupt--;
    }

    void handle_page_fault(CPU::fault_info* info, CPU* cpu, Proc::Thread* thread) {
        size_t cr2, cr3;

        asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");
        asm volatile("mov rax, cr3; mov %0, rax;" : : "m"(cr3) : "memory");

        AEX::printk("cpu%i, tid %i (b%i, c%i, i%i): Page fault @ 0x%x (0x%x)\n", cpu->id,
                    cpu->current_tid, thread->_busy, thread->_critical, cpu->in_interrupt, cr2,
                    cr3);
        AEX::printk("RIP: 0x%p\n", info->rip);

        Debug::stack_trace();

        auto state = out(thread, cpu);

        Proc::Thread::sleep(500);

        in(state, thread, cpu);
    }
}