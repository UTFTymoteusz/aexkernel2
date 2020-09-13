#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/utility.hpp"

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

    bool handle_page_fault(CPU::fault_info* info, CPU* cpu, Proc::Thread* thread);

    inline Proc::Thread::state out(Proc::Thread* thread, CPU*& cpu);
    inline void                in(Proc::Thread::state& state, Proc::Thread* thread, CPU*& cpu);

    extern "C" void common_fault_handler(void* m_info) {
        CPU::current()->in_interrupt++;

        auto info   = (CPU::fault_info*) m_info;
        auto cpu    = CPU::current();
        auto thread = cpu->current_thread;

        AEX_ASSERT_PEDANTIC(!CPU::checkInterrupts());

        switch (info->int_no) {
        case EXC_PAGE_FAULT:
            if (!handle_page_fault(info, cpu, thread))
                break;

            CPU::current()->in_interrupt--;
            return;
        default:
            break;
        }

        // We might be on a different core by now
        cpu = CPU::current();

        int  delta = 0;
        auto name  = Debug::addr2name((void*) info->rip, delta);
        if (!name)
            name = "no idea";

        switch (info->int_no) {
        case EXC_DEBUG:
        case EXC_NMI:
            printk(PRINTK_WARN
                   "cpu%i: %93$%s%$ Exception (%i) (%93$%i%$)\nRIP: 0x%016lx <%s+0x%x>\n",
                   CPU::currentID(), exception_names[info->int_no], info->int_no, info->err,
                   info->rip, name, delta);
            break;

        default:
            printk(PRINTK_FAIL
                   "cpu%i: %93$%s%$ Exception (%i) (%91$%i%$)\nRIP: 0x%016lx <%s+0x%x>\n",
                   CPU::currentID(), exception_names[info->int_no], info->int_no, info->err,
                   info->rip, name, delta);

            break;
        }

        printk("TID: %8i (b%i, c%i, i%i)\n", cpu->unused, 2137, 2137, cpu->in_interrupt);

        if (info->int_no == EXC_PAGE_FAULT) {
            size_t cr2;

            asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");

            printk("%91$%s, %s, %s%$\n", (info->err & 0x04) ? "User" : "Kernel",
                   (info->err & 0x02) ? "Write" : "Read",
                   (info->err & 0x01) ? "Present" : "Not Present");

            printk("ssssss: 0x%lx\n", cr2);
            printk("actual: 0x%lx\n", thread->parent->pagemap->rawof((void*) cr2));
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

        printk("USTK: 0x%p (%i, 0x%p)\n", thread->user_stack, thread->user_stack_size,
               thread->user_stack + thread->user_stack_size);
        printk("KSTK: 0x%p (%i, 0x%p)\n", thread->kernel_stack, thread->kernel_stack_size,
               thread->kernel_stack + thread->kernel_stack_size);
        printk("FSTK: 0x%p (%i, 0x%p)\n", thread->fault_stack, thread->fault_stack_size,
               thread->fault_stack + thread->fault_stack_size);

        if (info->int_no == EXC_DEBUG) {
            Debug::stack_trace();

            static int counter = 0;

            printk("counter: %i\n", counter);

            counter++;

            for (volatile size_t i = 0; i < 484354325; i++)
                ;

            CPU::current()->in_interrupt--;
            return;
        }

        if (info->int_no == EXC_NMI) {
            Debug::stack_trace();

            CPU::current()->in_interrupt--;
            return;
        }

        kpanic("Unrecoverable processor exception occured in CPU %i", CPU::currentID());

        CPU::current()->in_interrupt--;
    }

    bool handle_page_fault(UNUSED CPU::fault_info* info, CPU* cpu, Proc::Thread* thread) {
        auto state = out(thread, cpu);

        size_t cr2, cr3;

        asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");
        asm volatile("mov rax, cr3; mov %0, rax;" : : "m"(cr3) : "memory");

        void* addr = (void*) cr2;

        // AEX::printk("cpu%i, tid %i (b%i, c%i, i%i): Page fault @ 0x%lx (0x%lx)\n", cpu->id,
        //             cpu->unused, thread->m_busy, thread->m_critical, cpu->in_interrupt, cr2,
        //             cr3);

        // AEX::printk("RIP: 0x%p <%s+0x%x>\n", info->rip, name, delta);
        // Debug::stack_trace();

        /*AEX::printk("cpu%i, tid %i (b%i, c%i, i%i): Page fault @ 0x%lx (0x%lx)\n"
                    "RIP: 0x%016lx <%s+0x%x>\n",
                    cpu->id, cpu->unused, thread->m_busy, thread->m_critical,
           cpu->in_interrupt, cr2, cr3, info->rip, name);  */

        auto process = thread->getProcess();
        auto region  = Mem::find_mmap_region(process, addr);
        if (!region) {
            /*AEX::printk("cpu%i, tid %i (b%i, c%i, i%i): Unrecoverable page fault @
               0x%lx (0x%lx)\n" "RIP: 0x%016lx <%s+0x%x>\n", cpu->id,
               cpu->unused, thread->m_busy, thread->m_critical,
                        cpu->in_interrupt, cr2, cr3, info->rip, name);*/

            in(state, thread, cpu);
            return false;
        }

        size_t aligned = int_floor(cr2, (size_t) CPU::PAGE_SIZE);
        size_t offset  = aligned - (size_t) region->start;

        region->read((void*) aligned, offset, CPU::PAGE_SIZE);
        in(state, thread, cpu);
        return true;
    }

    inline Proc::Thread::state out(Proc::Thread* thread, CPU*& cpu) {
        auto state = thread->saveState();

        thread->setBusy(1);
        thread->setCritical(0);
        thread->setStatus(Proc::TS_RUNNABLE);

        cpu->in_interrupt--;
        CPU::interrupts();

        return state;
    }

    inline void in(Proc::Thread::state& state, Proc::Thread* thread, CPU*& cpu) {
        CPU::nointerrupts();
        cpu = CPU::current();
        cpu->in_interrupt++;

        thread->loadState(state);
    }
}