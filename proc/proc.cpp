#include "proc/proc.hpp"

#include "aex/arch/proc/context.hpp"
#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/config.hpp"
#include "aex/debug.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

#include "proc/broker.hpp"
#include "sys/mcore.hpp"
#include "sys/time.hpp"

using namespace AEX::Mem;
using namespace AEX::Sys;

namespace AEX::Proc {
    Mutex processes_lock;

    int      process_list_size;
    Process* process_list_head;
    Process* process_list_tail;

    Spinlock sched_lock;

    int     thread_list_size = 0;
    Thread* thread_list_head = nullptr;
    Thread* thread_list_tail = nullptr;

    Thread** idle_threads = nullptr;

    bool ready = false;

    Thread** void_threads;

    const char* status_names[] = {
        "TS_FRESH", "TS_RUNNABLE", "TS_SLEEPING", "TS_BLOCKED", "TS_VERYSPECIAL", "TS_DEAD",
    };

    void setup_idles();
    void setup_cores(Thread* bsp_thread);
    void cleanup_voids();

    void init() {
        void_threads = new Thread*[MCore::cpu_count];

        // auto& bsp = MCore::CPUs[0];

        auto idle_process   = new Process(KERNEL_PATH, 0, Mem::kernel_pagemap, "idle");
        auto kernel_process = new Process(KERNEL_PATH, 0, Mem::kernel_pagemap);

        idle_process->set_cwd("/");
        kernel_process->set_cwd("/");

        idle_process->session   = kernel_process->pid;
        idle_process->group     = kernel_process->pid;
        kernel_process->session = kernel_process->pid;
        kernel_process->group   = kernel_process->pid;

        ASSERT(kernel_process->isSessionLeader());
        ASSERT(kernel_process->isGroupLeader());

        idle_process->ready();
        kernel_process->ready();

        auto bsp_thread = new Thread(kernel_process);
        bsp_thread->fault_stack.ptr =
            (void*) ((size_t) Mem::kernel_pagemap->alloc(Thread::FAULT_STACK_SIZE, PAGE_WRITE) +
                     Thread::FAULT_STACK_SIZE);
        bsp_thread->fault_stack.size = Thread::FAULT_STACK_SIZE;
        bsp_thread->status           = TS_RUNNABLE;

        bsp_thread->original_entry = nullptr;
        // printk("aa %p\n", bsp_thread);
        // printk("bb %p\n", &bsp_thread->context->rip);
        // printk("cc %p\n", &bsp_thread->context->padding);
        // printk("ss %p\n", &bsp_thread->context->ss);
        // printk("cs %p\n", &bsp_thread->context->cs);
        // printk("fx %p\n", &bsp_thread->context->fxstate);
        // printk("int %p\n", Sys::MCore::CPUs[0]->reshed_stack);

        // bsp->previous_thread = bsp_thread;
        // bsp->current_thread  = bsp_thread;
        // bsp->current_context = bsp_thread->context;

        thread_list_size       = 1;
        thread_list_head       = bsp_thread;
        thread_list_head->next = bsp_thread;
        thread_list_head->prev = bsp_thread;
        thread_list_tail       = bsp_thread;

        kernel_process->threads.push(bsp_thread);

#if KPANIC_PROC_TRACE
        kpanic_hook.subscribe([]() {
            printk("Stack trace:\n");
            Debug::stack_trace(1);
        });
#endif

#if KPANIC_PROC_LIST
        kpanic_hook.subscribe([]() {
            printk("Processes:\n");
            Proc::debug_print_processes();

            printk("Threads:\n");
            Proc::debug_print_threads();

            printk("Idles:\n");
            Proc::debug_print_idles();

            printk("Processors:\n");
            for (int i = 0; i < MCore::cpu_count; i++) {
                auto cpu = Sys::MCore::CPUs[i];
                printk("%3i. %p\n", i, cpu->current_thread);
            }
        });
#endif

        broker_init();

        setup_idles();
        setup_cores(bsp_thread);
        setup_irq();
        cleanup_voids();

        ready = true;

        Proc::Thread::yield();
    }

    pid_t add_process(Process* process) {
        static pid_t counter = 0;

        ASSERT(process);
        ASSERT(!processes_lock.tryAcquire());

        auto scope = sched_lock.scope();

        if (process_list_head == nullptr) {
            process_list_size++;

            process_list_head = process;
            process_list_tail = process;

            process->next = process;
            process->prev = process;

            return counter++;
        }

        process_list_size++;
        process_list_tail->next = process;

        process->next = process_list_head;
        process->prev = process_list_tail;

        process_list_tail       = process;
        process_list_head->prev = process_list_tail;

        return counter++;
    }

    void remove_process(Process* process) {
        ASSERT(process);
        ASSERT(!processes_lock.tryAcquire());

        auto scope = sched_lock.scope();

        ASSERT(process_list_size > 0);

        process_list_size--;

        if (process_list_head == process)
            process_list_head = process->next;

        if (process_list_tail == process)
            process_list_tail = process->prev;

        process->next->prev = process->prev;
        process->prev->next = process->next;
    }

    Process* get_process(pid_t pid) {
        ASSERT(!processes_lock.tryAcquire());

        auto scope   = sched_lock.scope();
        auto process = process_list_head;

        for (int i = 0; i < process_list_size; i++) {
            if (process->pid == pid)
                return process;

            process = process->next;
        }

        return nullptr;
    }

    void add_thread(Thread* thread) {
        SCOPE(sched_lock);

        ASSERT(thread_list_size > 0);

        thread_list_size++;
        thread_list_tail->next = thread;

        thread->next = thread_list_head;
        thread->prev = thread_list_tail;

        thread_list_tail       = thread;
        thread_list_head->prev = thread_list_tail;
    }

    void remove_thread(Thread* thread) {
        SCOPE(sched_lock);

        ASSERT(thread->held_mutexes == 0);
        ASSERT(thread_list_size > 0);

        for (int i = 0; i < MCore::cpu_count; i++)
            if (idle_threads[i]->next == thread)
                idle_threads[i]->next = thread->next;

        thread_list_size--;

        if (thread_list_head == thread)
            thread_list_head = thread->next;

        if (thread_list_tail == thread)
            thread_list_tail = thread->prev;

        thread->next->prev = thread->prev;
        thread->prev->next = thread->next;

        // TODO: Figure out a better solution
        thread->next = nullptr;
    }

    void idle() {
        while (true)
            CPU::wait();
    }

    void setup_idles() {
        idle_threads = (Thread**) new Thread*[MCore::cpu_count];

        for (int i = 0; i < MCore::cpu_count; i++) {
            idle_threads[i] =
                Thread::create(0, (void*) idle, 512, Mem::kernel_pagemap, false, true).value;
            idle_threads[i]->status = TS_VERYSPECIAL;
        }
    }

    void setup_cores(Thread* bsp_thread) {
        for (int i = 1; i < MCore::cpu_count; i++) {
            auto void_thread = new Thread();
            void_threads[i]  = void_thread;

            MCore::CPUs[i]->previous_thread = void_thread;
            MCore::CPUs[i]->current_thread  = void_thread;
            MCore::CPUs[i]->current_context = void_thread->context;
            MCore::CPUs[i]->swthread(void_thread);

            void_thread->lock.acquireRaw();
            void_thread->next   = thread_list_head;
            void_thread->status = TS_DEAD;
        }

        MCore::CPUs[0]->previous_thread = bsp_thread;
        MCore::CPUs[0]->current_thread  = bsp_thread;
        MCore::CPUs[0]->current_context = bsp_thread->context;
        MCore::CPUs[0]->swthread(bsp_thread);

        // The scheduler will release the lock
        bsp_thread->lock.acquireRaw();
    }

    void cleanup_voids() {
        CPU::broadcast(CPU::IPP_RESHED);

        for (int i = 0; i < MCore::cpu_count; i++) {
            while (MCore::CPUs[i]->current_thread == void_threads[i])
                CPU::wait();

            delete void_threads[i];
        }

        delete void_threads;
    }

    const char* strstatus(status_t status) {
        return status_names[status];
    }
}