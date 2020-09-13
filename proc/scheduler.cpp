#include "aex/assert.hpp"
#include "aex/proc.hpp"
#include "aex/sys/time.hpp"

#include "proc/proc.hpp"
#include "sys/time.hpp"

using namespace AEX::Mem;
using namespace AEX::Sys;

namespace AEX::Proc {
    void schedule() {
        auto cpu    = CPU::current();
        auto thread = Thread::current();

        if (thread->isCritical())
            return;

        if (!lock.tryAcquireRaw()) {
            if (!(thread->status & TF_RUNNABLE))
                lock.acquireRaw();
            else
                return;
        }

        auto curtime = Time::uptime_raw();
        auto delta   = curtime - cpu->measurement_start_ns;

        cpu->measurement_start_ns = curtime;

        thread->parent->usage.cpu_time_ns += delta;
        thread->lock.releaseRaw();

        for (int i = 0; i <= thread_list_size; i++) {
            thread = thread->next;

            auto& status = thread->status;
            if (!(status & TF_RUNNABLE))
                continue;

            if (status & TF_BLOCKED) {
                if (!thread->aborting())
                    continue;
            }

            if (status & TF_SLEEPING) {
                if (curtime < thread->wakeup_at)
                    continue;

                status = TS_RUNNABLE;
            }

            if (thread->parent->cpu_affinity.isMasked(cpu->id))
                continue;

            if (!thread->lock.tryAcquireRaw())
                continue;

            cpu->current_thread  = thread;
            cpu->current_context = thread->context;

            cpu->update(thread);
            lock.releaseRaw();

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
        lock.releaseRaw();
    }
}