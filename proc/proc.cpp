#include "proc/proc.hpp"

#include "aex/mem/heap.hpp"
#include "aex/mem/smartarray.hpp"
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
    Thread**                 threads      = nullptr;
    Thread**                 idle_threads = nullptr;

    int thread_array_count = 0;

    Thread void_thread;

    Spinlock lock;

    void setup_idle_threads(Process* process);
    void setup_cores(Thread* bsp_thread);

    void init() {
        auto idle_process   = new Process("/sys/aexkrnl.elf", 0, "idle");
        auto kernel_process = new Process("/sys/aexkrnl.elf", 0);

        auto bsp_thread = new Thread(kernel_process);
        bsp_thread->start();

        tid_t tid = add_thread(bsp_thread);
        kernel_process->threads.pushBack(tid);

        setup_idle_threads(idle_process);
        setup_cores(bsp_thread);
        setup_irq();
    }

    void schedule() {
        auto cpu = Sys::CPU::getCurrentCPU();

        if (!lock.tryAcquire()) {
            if (cpu->currentThread->status != Thread::state::RUNNABLE)
                lock.acquire();
            else
                return;
        }

        int i = cpu->current_tid;

        uint64_t curtime = Sys::IRQ::get_curtime();
        uint64_t delta   = curtime - cpu->measurement_start_ns;

        cpu->measurement_start_ns = curtime;

        cpu->currentThread->parent->usage.cpu_time_ns += delta;
        cpu->currentThread->lock.release();

        auto increment = [&i]() {
            i++;
            if (i >= thread_array_count)
                i = 0;
        };

        increment();

        for (int j = 0; j < thread_array_count; j++) {
            if (!threads[i]) {
                increment();
                continue;
            }

            switch (threads[i]->status) {
            case Thread::state::RUNNABLE:
                break;
            case Thread::state::SLEEPING:
                if (Sys::IRQ::get_curtime() < threads[i]->wakeup_at) {
                    increment();
                    continue;
                }

                threads[i]->status = Thread::state::RUNNABLE;

                break;
            default:
            case Thread::state::BLOCKED:
                increment();
                continue;
            }

            if (threads[i]->parent->cpu_affinity.isMasked(cpu->id)) {
                increment();
                continue;
            }

            if (!threads[i]->lock.tryAcquire()) {
                increment();
                continue;
            }

            cpu->current_tid    = i;
            cpu->currentThread  = threads[i];
            cpu->currentContext = &threads[i]->context;

            lock.release();

            return;
        }

        cpu->current_tid    = i;
        cpu->currentThread  = idle_threads[cpu->id];
        cpu->currentContext = &idle_threads[cpu->id]->context;

        cpu->currentThread->lock.tryAcquire();

        lock.release();
    }

    int add_process(Process* process) {
        return processes.addRef(process);
    }

    int add_thread(Thread* thread) {
        auto scopeLock = ScopeSpinlock(lock);

        for (int i = 0; i < thread_array_count; i++) {
            if (threads[i])
                continue;

            threads[i] = thread;
            return i;
        }

        thread_array_count++;

        threads = (Thread**) Heap::realloc(threads, thread_array_count * sizeof(Thread*));
        threads[thread_array_count - 1] = thread;

        return thread_array_count - 1;
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

            idle_threads[i]->lock.acquire();
        }

        Sys::MCore::CPUs[0]->current_tid    = 1;
        Sys::MCore::CPUs[0]->currentThread  = bsp_thread;
        Sys::MCore::CPUs[0]->currentContext = &bsp_thread->context;

        // The scheduler will release the lock
        bsp_thread->lock.acquire();
    }
}