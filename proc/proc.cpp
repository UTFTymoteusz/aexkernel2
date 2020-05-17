#include "proc/proc.hpp"

#include "aex/ipc/messagequeue.hpp"
#include "aex/mem/heap.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"

#include "proc/context.hpp"
#include "sys/cpu.hpp"
#include "sys/irq.hpp"
#include "sys/mcore.hpp"

namespace AEX::Proc {
    Mem::SmartArray<Process> processes;

    IPC::MessageQueue threads_to_reap;

    Thread** threads      = nullptr;
    Thread** idle_threads = nullptr;

    int thread_array_size = 0;

    Thread void_thread;

    Spinlock lock;

    void setup_idle_threads(Process* process);
    void setup_cores(Thread* bsp_thread);

    void thread_reaper();

    void init() {
        threads_to_reap = IPC::MessageQueue();

        threads           = (Thread**) Heap::malloc(sizeof(Thread*) * 1);
        thread_array_size = 1;

        auto idle_process   = new Process("/sys/aexkrnl.elf", 0, VMem::kernel_pagemap, "idle");
        auto kernel_process = new Process("/sys/aexkrnl.elf", 0, VMem::kernel_pagemap);

        auto bsp_thread = new Thread(kernel_process);
        bsp_thread->start();

        add_thread(bsp_thread);

        bsp_thread->refs->increment();
        kernel_process->threads.addRef(bsp_thread, bsp_thread->refs);

        auto thread_reaper_thread =
            new Thread(kernel_process, (void*) thread_reaper, VMem::kernel_pagemap->alloc(8192),
                       8192, kernel_process->pagemap);
        thread_reaper_thread->start();

        setup_idle_threads(idle_process);
        setup_cores(bsp_thread);
        setup_irq();
    }

    void schedule() {
        auto cpu = Sys::CPU::getCurrentCPU();

        if (cpu->currentThread->isCritical())
            return;

        if (!lock.tryAcquireRaw()) {
            if (cpu->currentThread->status != Thread::status_t::RUNNABLE)
                lock.acquireRaw();
            else
                return;
        }

        int i = cpu->current_tid;

        uint64_t curtime = Sys::IRQ::get_uptime();
        uint64_t delta   = curtime - cpu->measurement_start_ns;

        cpu->measurement_start_ns = curtime;

        cpu->currentThread->parent->usage.cpu_time_ns += delta;
        cpu->currentThread->lock.releaseRaw();

        auto increment = [&i]() {
            i++;
            if (i >= thread_array_size)
                i = 1;
        };

        increment();

        for (int j = 1; j < thread_array_size; j++) {
            if (!threads[i]) {
                increment();
                continue;
            }

            switch (threads[i]->status) {
            case Thread::status_t::RUNNABLE:
                break;
            case Thread::status_t::SLEEPING:
                if (curtime < threads[i]->wakeup_at) {
                    increment();
                    continue;
                }

                threads[i]->status = Thread::status_t::RUNNABLE;

                break;
            case Thread::status_t::BLOCKED:
                if (threads[i]->isAbortSet())
                    break;

                increment();
                continue;
            default:
                increment();
                continue;
            }

            if (threads[i]->parent->cpu_affinity.isMasked(cpu->id)) {
                increment();
                continue;
            }

            if (!threads[i]->lock.tryAcquireRaw()) {
                increment();
                continue;
            }

            cpu->current_tid    = i;
            cpu->currentThread  = threads[i];
            cpu->currentContext = &threads[i]->context;

            lock.releaseRaw();

            return;
        }

        cpu->current_tid    = i;
        cpu->currentThread  = idle_threads[cpu->id];
        cpu->currentContext = &idle_threads[cpu->id]->context;

        cpu->currentThread->lock.tryAcquireRaw();

        lock.releaseRaw();
    }

    pid_t add_process(Process* process) {
        return processes.addRef(process);
    }

    tid_t add_thread(Thread* thread) {
        auto scopeLock = ScopeSpinlock(lock);

        for (int i = 1; i < thread_array_size; i++) {
            if (threads[i])
                continue;

            threads[i] = thread;
            return i;
        }

        thread_array_size++;
        threads = (Thread**) Heap::realloc(threads, thread_array_size * sizeof(Thread*));

        threads[thread_array_size - 1] = thread;
        thread->tid                    = thread_array_size - 1;

        return thread_array_size - 1;
    }

    void abort_thread(Thread* thread) {
        {
            auto process = thread->getProcess();
            for (auto iterator = process->threads.getIterator(); auto _thread = iterator.next();) {
                if (_thread == thread) {
                    process->threads.remove(iterator.index());
                    break;
                }
            }
        }

        thread->announceExit();

        reap_thread(thread);

        if (thread == Thread::getCurrentThread())
            while (true)
                ;
    }

    void reap_thread(Thread* thread) {
        {
            auto scopeLock = ScopeSpinlock(lock);

            if (thread->tid == -1)
                return;

            threads[thread->tid] = nullptr;
            thread->tid          = -1;
        }

        threads_to_reap.writeObject(thread);
    }

    void idle() {
        while (true)
            Sys::CPU::waitForInterrupt();
    }

    void setup_idle_threads(Process* idle_process) {
        idle_threads = (Thread**) Heap::malloc(sizeof(Thread*) * Sys::MCore::cpu_count);

        for (int i = 0; i < Sys::MCore::cpu_count; i++) {
            void* stack = Heap::malloc(1024);

            idle_threads[i] = new Thread(idle_process, (void*) idle, stack, 1024,
                                         VMem::kernel_pagemap, false, true);
        }
    }

    void setup_cores(Thread* bsp_thread) {
        for (int i = 1; i < Sys::MCore::cpu_count; i++) {
            Sys::MCore::CPUs[i]->currentThread  = idle_threads[i];
            Sys::MCore::CPUs[i]->currentContext = &void_thread.context;
            Sys::MCore::CPUs[i]->current_tid    = 0;

            idle_threads[i]->lock.acquireRaw();
        }

        Sys::MCore::CPUs[0]->current_tid    = 1;
        Sys::MCore::CPUs[0]->currentThread  = bsp_thread;
        Sys::MCore::CPUs[0]->currentContext = &bsp_thread->context;

        // The scheduler will release the lock
        bsp_thread->lock.acquireRaw();
    }

    void thread_reaper() {
        while (true) {
            auto thread = threads_to_reap.readObject<Thread*>();

            thread->lock.acquire();
            thread->setStatus(Thread::status_t::DEAD);

            if (thread->refs->decrement())
                delete thread;
        }
    }
}