#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/mem/mmap.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/utility.hpp"

#include "proc/proc.hpp"

constexpr auto EXC_DEBUG          = 1;
constexpr auto EXC_NMI            = 2;
constexpr auto EXC_INVALID_OPCODE = 6;
constexpr auto EXC_PAGE_FAULT     = 14;

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
    bool handle_invalid_opcode(CPU::fault_info* info, CPU* cpu, Proc::Thread* thread);
    void print_info(CPU::fault_info* info);

    inline Proc::Thread::state out(Proc::Thread* thread, CPU*& cpu);
    inline void                in(Proc::Thread::state& state, Proc::Thread* thread, CPU*& cpu);

    extern "C" void common_fault_handler(void* m_info) {
        CPU::current()->in_interrupt++;

        auto info   = (CPU::fault_info*) m_info;
        auto cpu    = CPU::current();
        auto thread = cpu->current_thread;

        thread->addBusy();

        AEX_ASSERT_PEDANTIC(!CPU::checkInterrupts());

        if (thread->faulting) {
            printk("recursive fault @ 0x%p\n", info->rip);
            Debug::stack_trace();

            CPU::broadcast(CPU::IPP_HALT);
            CPU::wait();
        }

        thread->faulting = true;

        switch (info->int_no) {
        case EXC_PAGE_FAULT:
            if (!handle_page_fault(info, cpu, thread))
                break;

            CPU::current()->in_interrupt--;

            thread->faulting = false;
            thread->subBusy();

            return;
        case EXC_INVALID_OPCODE:
            if (!handle_invalid_opcode(info, cpu, thread))
                break;

            CPU::current()->in_interrupt--;

            thread->faulting = false;
            thread->subBusy();

            return;
        default:
            break;
        }

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

            Proc::debug_print_threads();
            Proc::debug_print_processes();
            break;
        default:
            printk(PRINTK_FAIL
                   "cpu%i: %93$%s%$ Exception (%i) (%91$%i%$)\nRIP: 0x%016lx <%s+0x%x>\n",
                   CPU::currentID(), exception_names[info->int_no], info->int_no, info->err,
                   info->rip, name, delta);

            break;
        }

        printk("PID: %i THPTR: 0x%p (b%i, c%i, i%i)\n", thread->parent->pid, thread, 2137, 2137,
               cpu->in_interrupt);

        if (info->int_no == EXC_PAGE_FAULT) {
            size_t cr2;

            asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");

            printk("%91$%s, %s, %s%$\n", (info->err & 0x04) ? "User" : "Kernel",
                   (info->err & 0x02) ? "Write" : "Read",
                   (info->err & 0x01) ? "Present" : "Not Present");

            printk("ssssss: 0x%lx\n", cr2);
            printk("actual: 0x%lx\n", thread->parent->pagemap->rawof((void*) cr2));
        }

        print_info(info);

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

            thread->faulting = false;
            thread->subBusy();

            CPU::current()->in_interrupt--;
            return;
        }

        if (info->int_no == EXC_NMI) {
            Debug::stack_trace();

            Proc::debug_print_processes();
            Proc::debug_print_threads();

            thread->faulting = false;
            thread->subBusy();

            CPU::current()->in_interrupt--;
            return;
        }

        kpanic("Unrecoverable processor exception occured in CPU %i", CPU::currentID());

        thread->faulting = false;
        thread->subBusy();

        CPU::current()->in_interrupt--;
    }

    bool handle_page_fault(UNUSED CPU::fault_info* info, CPU* cpu, Proc::Thread* thread) {
        auto state = out(thread, cpu);

        size_t cr2, cr3;

        asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");
        asm volatile("mov rax, cr3; mov %0, rax;" : : "m"(cr3) : "memory");

        void* addr = (void*) cr2;

        auto process = (info->err & 0x04) ? thread->getProcess() : Proc::Process::kernel();
        auto region  = Mem::find_mmap_region(process, addr);
        if (!region) {
            if (process == Proc::Process::kernel()) {
                in(state, thread, cpu);
                return false;
            }

            // AEX::printk("cpu%i: Page fault @ 0x%lx (0x%lx)\n"
            //            "    RIP: 0x%016lx\n",
            //            cpu->id, cr2, cr3, info->rip);

            IPC::siginfo_t sinfo = {};

            sinfo.si_signo = IPC::SIGSEGV;
            sinfo.si_code  = (info->err & 0x01) ? SEGV_MAPERR : SEGV_ACCERR;
            sinfo.si_addr  = addr;

            thread->signal(sinfo);

            in(state, thread, cpu);
            return true;
        }

        size_t aligned = int_floor(cr2, (size_t) CPU::PAGE_SIZE);
        size_t offset  = aligned - (size_t) region->start;

        region->read((void*) aligned, offset, CPU::PAGE_SIZE);

        in(state, thread, cpu);
        return true;
    }

    bool handle_invalid_opcode(CPU::fault_info* info, CPU* cpu, Proc::Thread* thread) {
        auto state = out(thread, cpu);

        if (info->cs == 0x08) {
            in(state, thread, cpu);
            return false;
        }

        // AEX::printk("cpu%i: Invalid opcode @ 0x%lx (0x%lx)\n"
        //            "    RIP: 0x%016lx\n",
        //            cpu->id, cr2, cr3, info->rip);

        IPC::siginfo_t sinfo = {};

        sinfo.si_signo = IPC::SIGILL;
        sinfo.si_code  = ILL_ILLOPC;
        sinfo.si_addr  = (void*) info->rip;

        thread->signal(sinfo);

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

    void print_info(CPU::fault_info* info) {
        printk("RAX: 0x%016lx  RBX: 0x%016lx  RCX: 0x%016lx  RDX: 0x%016lx\n", info->rax, info->rbx,
               info->rcx, info->rdx);
        printk("RSI: 0x%016lx  RDI: 0x%016lx  RSP: 0x%016lx  RBP: 0x%016lx\n", info->rsi, info->rdi,
               info->rsp, info->rbp);

        printk("R8 : 0x%016lx  R9 : 0x%016lx  R10: 0x%016lx  R11: 0x%016lx\n", info->r8, info->r9,
               info->r10, info->r11);
        printk("R12: 0x%016lx  R13: 0x%016lx  R14: 0x%016lx  R15: 0x%016lx\n", info->r12, info->r13,
               info->r14, info->r15);

        int   delta;
        auto* name = Debug::addr2name((void*) info->rip, delta);
        if (!name)
            name = "no idea";

        printk("RIP: 0x%p <%s+0x%x>\n", info->rip, name, delta);
        printk("RFLAGS: 0x%016lx\n", info->rflags);

        size_t cr0, cr2, cr3, cr4;

        asm volatile("mov rax, cr0; mov %0, rax;" : : "m"(cr0) : "memory");
        asm volatile("mov rax, cr2; mov %0, rax;" : : "m"(cr2) : "memory");
        asm volatile("mov rax, cr3; mov %0, rax;" : : "m"(cr3) : "memory");
        asm volatile("mov rax, cr4; mov %0, rax;" : : "m"(cr4) : "memory");

        printk(
            "CR0: 0x%016lx  CR1: 0xUDUDUDUDUDUDUDUD  CR2: 0x%016lx  CR3: 0x%016lx  CR4: 0x%016lx\n",
            cr0, cr2, cr3, cr4);
    }
}