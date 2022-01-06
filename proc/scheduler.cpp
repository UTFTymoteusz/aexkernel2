#include "aex/assert.hpp"
#include "aex/proc.hpp"
#include "aex/sec/random.hpp"
#include "aex/sys/time.hpp"
#include "aex/utility.hpp"

#include "proc/proc.hpp"
#include "sys/time.hpp"

using namespace AEX::Mem;
using namespace AEX::Sys;

namespace AEX::Proc {
    void schedule() {
        auto cpu    = CPU::current();
        auto thread = Thread::current();

        if (thread->isCritical()) {
            cpu->should_yield = true;
            return;
        }

        if (thread->sched_counter >= 1000000) {
            thread->sched_counter -= 1000000;

            if (thread->status == TS_RUNNABLE) {
                Time::uptime();
                return;
            }
        }

        if (!sched_lock.tryAcquireRaw()) {
            if (thread->status != TS_RUNNABLE || thread->parent->stopped)
                sched_lock.acquireRaw();
            else
                return;
        }

        cpu->in_interrupt++;
        cpu->rescheduling = true;

        auto uptime = Time::uptime();
        auto delta  = uptime - cpu->measurement_start_ns;
        auto next   = thread->next;

        cpu->measurement_start_ns = uptime;

        thread->parent->usage.cpu_time_ns += delta;
        thread->lock.releaseRaw();

        Sec::feed_random(uptime);

        if (next == nullptr)
            thread = thread_list_head;

        for (int i = 0; i <= thread_list_size; i++) {
            thread = thread->next;

            switch (thread->status) {
            case TS_RUNNABLE:
                break;
            case TS_SLEEPING:
                if (uptime < thread->wakeup_at && !thread->interrupted())
                    continue;

                Sec::feed_random(thread->wakeup_at);

                thread->status = TS_RUNNABLE;
                break;
            case TS_BLOCKED:
                if (!thread->interrupted())
                    continue;

                break;
            default:
                continue;
            }

            auto process = thread->getProcess();
            if (process->cpu_affinity.masked(cpu->id))
                continue;

            if (process->stopped && !thread->isBusy())
                continue;

            auto counter = thread->sched_counter;

            if (process->nice < 0)
                counter += 1000000 + 50000 * -process->nice;
            else
                counter += 1000000 - 25000 * process->nice;

            if (counter < 1000000)
                continue;

            if (!thread->lock.tryAcquireRaw())
                continue;

            thread->sched_counter = counter - 1000000;

            cpu->previous_thread = cpu->current_thread;
            cpu->current_thread  = thread;
            cpu->current_context = thread->context;
            cpu->kernel_stack    = (size_t) thread->kernel_stack.ptr + thread->kernel_stack.size;
            cpu->syscall_table   = process->syscall_table;

            cpu->swthread(thread);

            if (!thread->isBusy() && thread->sigchk()) {
                if (process->stopped || (!thread->isBusy() && process->exiting())) {
                    thread->lock.releaseRaw();
                    continue;
                }
            }

            sched_lock.releaseRaw();

            cpu->rescheduling = false;
            cpu->in_interrupt--;

            return;
        }

        auto idle = idle_threads[cpu->id];
        if (thread == idle)
            thread = thread_list_head;

        idle->lock.tryAcquireRaw();

        cpu->current_thread  = idle;
        cpu->current_context = idle->context;

        idle->next = thread;

        cpu->swthread(idle);
        sched_lock.releaseRaw();

        cpu->rescheduling = false;
        cpu->in_interrupt--;
    }
}