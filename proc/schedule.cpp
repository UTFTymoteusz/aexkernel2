#include "aex/proc.hpp"
#include "aex/sys/time.hpp"

#include "sys/time.hpp"

using namespace AEX::Mem;
using namespace AEX::Sys;

namespace AEX::Proc {
    extern Spinlock lock;

    extern Thread** threads;
    extern Thread** idle_threads;

    extern int thread_array_size;

    void schedule() {
        auto cpu = CPU::current();

        if (cpu->current_thread->isCritical())
            return;

        if (!lock.tryAcquireRaw()) {
            if (!(cpu->current_thread->status & TF_RUNNABLE))
                lock.acquireRaw();
            else
                return;
        }

        tid_t i = cpu->current_tid;

        auto curtime = Time::uptime_raw();
        auto delta   = curtime - cpu->measurement_start_ns;

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
                if (!threads[i]->isAbortSet())
                    continue;
            }

            if (status & TF_SLEEPING) {
                if (curtime < threads[i]->wakeup_at)
                    continue;

                status = TS_RUNNABLE;
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
}