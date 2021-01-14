#include "aex/assert.hpp"
#include "aex/proc.hpp"
#include "aex/sys/time.hpp"
#include "aex/utility.hpp"

#include "proc/proc.hpp"
#include "sys/time.hpp"

using namespace AEX::Mem;
using namespace AEX::Sys;

namespace AEX::Proc {
    WEAK void schedule() {
        auto cpu    = CPU::current();
        auto thread = Thread::current();

        if (thread->isCritical())
            return;

        if (!sched_lock.tryAcquireRaw()) {
            if (thread->status != TS_RUNNABLE)
                sched_lock.acquireRaw();
            else
                return;
        }

        auto uptime = Time::uptime_raw();
        auto delta  = uptime - cpu->measurement_start_ns;

        cpu->measurement_start_ns = uptime;

        thread->parent->usage.cpu_time_ns += delta;
        thread->lock.releaseRaw();

        for (int i = 0; i <= thread_list_size; i++) {
            thread = thread->next;

            switch (thread->status) {
            case TS_RUNNABLE:
                break;
            case TS_BLOCKED:
                if (!thread->interrupted())
                    continue;

                break;
            case TS_SLEEPING:
                if (uptime < thread->wakeup_at && !thread->interrupted())
                    continue;

                thread->status = TS_RUNNABLE;
                break;
            default:
                continue;
            }

            if (thread->parent->cpu_affinity.masked(cpu->id))
                continue;

            if (!thread->lock.tryAcquireRaw())
                continue;

            cpu->current_thread  = thread;
            cpu->current_context = thread->context;
            cpu->kernel_stack    = thread->kernel_stack + thread->kernel_stack_size;
            cpu->syscall_table   = thread->getProcess()->syscall_table;

            cpu->update(thread);
            sched_lock.releaseRaw();

            return;
        }

        auto idle = idle_threads[cpu->id];
        if (thread == idle)
            thread = thread_list_head;

        // We don't care, it's our private thread anyways
        idle->lock.tryAcquireRaw();

        cpu->current_thread  = idle;
        cpu->current_context = idle->context;

        idle->next = thread;

        cpu->update(idle);
        sched_lock.releaseRaw();
    }
}