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

    IPC::MessageQueue* threads_to_reap;

    Thread** threads      = nullptr;
    Thread** idle_threads = nullptr;

    bool ready = false;

    int thread_array_size = 0;

    Thread** void_threads;

    Spinlock lock;

    void setup_idle_threads(Process* process);
    void setup_cores(Thread* bsp_thread);
    void cleanup_voids();

    void thread_reaper();

    void init() {
        void_threads    = new Thread*[MCore::cpu_count];
        threads_to_reap = new IPC::MessageQueue();

        auto bsp = MCore::CPUs[0];

        threads           = (Thread**) new Thread*[1];
        thread_array_size = 1;

        auto idle_process   = new Process("/sys/aexkrnl.elf", 0, Mem::kernel_pagemap, "idle");
        auto kernel_process = new Process("/sys/aexkrnl.elf", 0, Mem::kernel_pagemap);

        auto bsp_thread = new Thread(kernel_process);
        bsp_thread->fault_stack =
            (size_t) Mem::kernel_pagemap->alloc(Thread::FAULT_STACK_SIZE, PAGE_WRITE) +
            Thread::FAULT_STACK_SIZE;
        bsp_thread->fault_stack_size = Thread::FAULT_STACK_SIZE;
        bsp_thread->start();

        bsp->current_thread  = bsp_thread;
        bsp->current_context = bsp_thread->context;
        bsp->current_tid     = bsp_thread->tid;

        add_thread(bsp_thread);

        bsp_thread->shared->increment();
        kernel_process->threads.addRef(bsp_thread, bsp_thread->shared);

        auto thread_reaper_thread =
            new Thread(kernel_process, (void*) thread_reaper, 8192, kernel_process->pagemap);
        thread_reaper_thread->start();

        setup_idle_threads(idle_process);
        setup_cores(bsp_thread);
        setup_irq();
        cleanup_voids();

        ready = true;

        Proc::Thread::yield();
    }

    void schedule() {
        auto cpu = CPU::getCurrent();

        if (!lock.tryAcquireRaw()) {
            if (cpu->current_thread->status != TS_RUNNABLE)
                lock.acquireRaw();
            else
                return;
        }

        int i = cpu->current_tid;

        Time::time_t curtime = Time::uptime_raw();
        Time::time_t delta   = curtime - cpu->measurement_start_ns;

        cpu->measurement_start_ns = curtime;

        cpu->current_thread->parent->usage.cpu_time_ns += delta;
        cpu->current_thread->lock.releaseRaw();

        auto increment = [&i]() {
            i++;
            if (i >= thread_array_size)
                i = 1;
        };

        for (int j = 1; j < thread_array_size; j++) {
            increment();
            if (!threads[i])
                continue;

            auto& status = threads[i]->status;
            if (!(status & TF_RUNNABLE))
                continue;

            if (status & TF_BLOCKED) {
                if (threads[i]->isAbortSet())
                    break;

                continue;
            }

            if (status & TF_SLEEPING) {
                if (curtime < threads[i]->wakeup_at)
                    continue;

                status = TS_RUNNABLE;
                break;
            }

            if (threads[i]->parent->cpu_affinity.isMasked(cpu->id))
                continue;

            if (!threads[i]->lock.tryAcquireRaw())
                continue;

            auto thread = threads[i];

            cpu->current_tid     = i;
            cpu->current_thread  = thread;
            cpu->current_context = thread->context;

            cpu->update(thread);
            lock.releaseRaw();

            return;
        }

        auto thread = idle_threads[cpu->id];

        // We don't care, it's our private thread anyways
        thread->lock.tryAcquireRaw();

        cpu->current_tid     = 0;
        cpu->current_thread  = thread;
        cpu->current_context = thread->context;

        cpu->update(thread);
        lock.releaseRaw();
    }

    pid_t add_process(Process* process) {
        return processes.addRef(process);
    }

    tid_t add_thread(Thread* thread) {
        ScopeSpinlock scopeLock(lock);

        for (int i = 1; i < thread_array_size; i++) {
            if (threads[i])
                continue;

            threads[i]  = thread;
            thread->tid = i;

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
            for (auto iterator = process->threads.getIterator(); auto m_thread = iterator.next();) {
                if (m_thread == thread) {
                    process->threads.remove(iterator.index());
                    break;
                }
            }
        }

        thread->announceExit();

        reap_thread(thread);

        if (thread == Thread::getCurrent())
            while (true)
                Proc::Thread::yield();
    }

    void reap_thread(Thread* thread) {
        {
            ScopeSpinlock scopeLock(lock);

            if (thread->tid == -1)
                return;

            threads[thread->tid] = nullptr;
            thread->tid          = -1;
        }

        threads_to_reap->writeObject(thread);
    }

    void idle() {
        while (true)
            CPU::waitForInterrupt();
    }

    void setup_idle_threads(Process* idle_process) {
        idle_threads = (Thread**) new Thread*[MCore::cpu_count];

        for (int i = 0; i < MCore::cpu_count; i++) {
            idle_threads[i] =
                new Thread(idle_process, (void*) idle, 1024, Mem::kernel_pagemap, false, true);
        }
    }

    void setup_cores(Thread* bsp_thread) {
        for (int i = 1; i < MCore::cpu_count; i++) {
            auto void_thread = new Thread();
            void_threads[i]  = void_thread;

            MCore::CPUs[i]->current_thread  = void_thread;
            MCore::CPUs[i]->current_context = void_thread->context;
            MCore::CPUs[i]->current_tid     = 2137 + i;

            void_thread->lock.acquireRaw();
        }

        MCore::CPUs[0]->current_tid     = 1;
        MCore::CPUs[0]->current_thread  = bsp_thread;
        MCore::CPUs[0]->current_context = bsp_thread->context;

        // The scheduler will release the lock
        bsp_thread->lock.acquireRaw();
    }

    void cleanup_voids() {
        CPU::broadcastPacket(CPU::IPP_RESHED, nullptr);

        for (int i = 1; i < MCore::cpu_count; i++) {
            while (MCore::CPUs[i]->current_tid >= 2137)
                ;

            delete void_threads[i];
        }

        delete void_threads;
    }

    void thread_reaper() {
        while (true) {
            auto thread = threads_to_reap->readObject<Thread*>();

            thread->lock.acquire();
            thread->setStatus(TS_DEAD);
            thread->lock.release();

            if (thread->shared->decrement())
                delete thread;
        }
    }

    void debug_print_cpu_jobs() {
        for (int i = 0; i < MCore::cpu_count; i++) {
            auto cpu = MCore::CPUs[i];

            void* addr = (void*) cpu->current_thread->context->rip;

            int         delta = 0;
            const char* name  = Debug::addr2name(addr, delta);

            printk("cpu%i: PID %8i, TID %8i @ 0x%p <%s+0x%x> (b%i, c%i, i%i)\n", i,
                   cpu->current_thread->parent->pid, cpu->current_tid, addr,
                   name ? name : "no idea", delta, cpu->current_thread->m_busy,
                   cpu->current_thread->m_critical, cpu->in_interrupt);
        }
    }
}