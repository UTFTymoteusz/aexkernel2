#include "proc/proc.hpp"

#include "aex/arch/proc/context.hpp"
#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/ipc/messagequeue.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"

#include "sys/mcore.hpp"
#include "sys/time.hpp"

using namespace AEX::Mem;
using namespace AEX::Sys;

namespace AEX::Proc {
    Mem::SmartArray<Process> processes;

    Thread** idle_threads = nullptr;

    Spinlock lock;

    int     thread_list_size = 0;
    Thread* thread_list_head = nullptr;
    Thread* thread_list_tail = nullptr;

    bool ready = false;

    Thread** void_threads;

    void setup_idle_threads(Process* process);
    void setup_cores(Thread* bsp_thread);
    void cleanup_voids();

    void init() {
        void_threads = new Thread*[MCore::cpu_count];

        auto bsp = MCore::CPUs[0];

        auto idle_process   = new Process("/sys/aexkrnl.elf", 0, Mem::kernel_pagemap, "idle");
        auto kernel_process = new Process("/sys/aexkrnl.elf", 0, Mem::kernel_pagemap);

        auto bsp_thread = new Thread(kernel_process);
        bsp_thread->fault_stack =
            (size_t) Mem::kernel_pagemap->alloc(Thread::FAULT_STACK_SIZE, PAGE_WRITE) +
            Thread::FAULT_STACK_SIZE;
        bsp_thread->fault_stack_size = Thread::FAULT_STACK_SIZE;
        bsp_thread->setStatus(TS_RUNNABLE);

        bsp->current_thread  = bsp_thread;
        bsp->current_context = bsp_thread->context;

        thread_list_size       = 1;
        thread_list_head       = bsp_thread;
        thread_list_head->next = bsp_thread;
        thread_list_head->prev = bsp_thread;
        thread_list_tail       = bsp_thread;

        // auto thread_reaper = Thread::create(kernel_process, (void*) thread_reaper_loop, 8192,
        //                                    kernel_process->pagemap);

        setup_idle_threads(idle_process);
        setup_cores(bsp_thread);
        setup_irq();
        cleanup_voids();

        ready = true;

        Proc::Thread::yield();
    }

    pid_t add_process(Process* process) {
        return processes.addRef(process);
    }

    void add_thread(Thread* thread) {
        auto scope = lock.scope();

        thread_list_size++;
        thread_list_tail->next = thread;

        thread->next = thread_list_head;
        thread->prev = thread_list_tail;

        thread_list_tail       = thread;
        thread_list_head->prev = thread_list_tail;
    }

    void idle() {
        while (true)
            CPU::waitForInterrupt();
    }

    void setup_idle_threads(Process* idle_process) {
        idle_threads = (Thread**) new Thread*[MCore::cpu_count];

        for (int i = 0; i < MCore::cpu_count; i++) {
            idle_threads[i] =
                Thread::create(idle_process, (void*) idle, 512, Mem::kernel_pagemap, false, true)
                    .value;
        }
    }

    void setup_cores(Thread* bsp_thread) {
        for (int i = 1; i < MCore::cpu_count; i++) {
            auto void_thread = new Thread();
            void_threads[i]  = void_thread;

            MCore::CPUs[i]->current_thread  = void_thread;
            MCore::CPUs[i]->current_context = void_thread->context;

            void_thread->lock.acquireRaw();
            void_thread->next = thread_list_head;
            void_thread->setStatus(TS_DEAD);
        }

        MCore::CPUs[0]->current_thread  = bsp_thread;
        MCore::CPUs[0]->current_context = bsp_thread->context;

        // The scheduler will release the lock
        bsp_thread->lock.acquireRaw();
    }

    void cleanup_voids() {
        CPU::broadcast(CPU::IPP_RESHED, nullptr);

        for (int i = 1; i < MCore::cpu_count; i++) {
            while ((volatile Thread*) MCore::CPUs[i]->current_thread == void_threads[i])
                ;

            delete void_threads[i];
        }

        delete void_threads;
    }

    void debug_print_cpu_jobs() {
        for (int i = 0; i < MCore::cpu_count; i++) {
            auto cpu = MCore::CPUs[i];

            void* addr = (void*) cpu->current_thread->context->rip;

            int         delta = 0;
            const char* name  = Debug::addr2name(addr, delta);

            printk("cpu%i: PID %8i, TID %8i @ 0x%p <%s+0x%x> (b%i, c%i, i%i)\n", i,
                   cpu->current_thread->parent->pid, cpu->unused, addr, name ? name : "no idea",
                   delta, cpu->current_thread->m_busy, cpu->current_thread->m_critical,
                   cpu->in_interrupt);
        }
    }

    void debug_print_list() {
        printk("Head: 0x%p, Count: %i\n", thread_list_head, thread_list_size);
        auto thread = thread_list_head;

        for (int i = 0; i < thread_list_size; i++) {
            printk("0x%p (p: 0x%p, n: 0x%p)\n", thread, thread->prev, thread->next);
            thread = thread->next;
        }

        printk("Tail: 0x%p\n", thread_list_tail);
    }
}