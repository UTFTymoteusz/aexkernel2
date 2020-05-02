#include "aex/proc/thread.hpp"

#include "aex/printk.hpp"
#include "aex/rcparray.hpp"

#include "proc/context.hpp"
#include "proc/proc.hpp"
#include "sys/cpu.hpp"
#include "sys/irq.hpp"

#include <stddef.h>

namespace AEX::Proc {
    extern "C" void proc_reshed();

    Thread::Thread(Process* parent) {
        status = FRESH;

        this->parent = parent;
    }

    Thread::Thread(Process* parent, void* entry, void* stack, size_t stack_size,
                   VMem::Pagemap* pagemap, bool usermode, bool dont_add) {
        context = Context(entry, stack, stack_size, pagemap, usermode);
        status  = FRESH;

        this->parent = parent;

        if (dont_add)
            return;

        tid_t tid = Proc::add_thread(this);

        parent->lock.acquire();
        parent->threads.pushBack(tid);
        parent->lock.release();
    }

    void Thread::start() {
        if (status == FRESH)
            status = RUNNABLE;
    }

    RCPArray<Process>::Pointer Thread::getProcess() {
        return processes.get(this->parent->pid);
    }

    void Thread::yield() {
        proc_reshed();
    }

    void Thread::sleep(int ms) {
        auto currentThread = Thread::getCurrentThread();

        currentThread->wakeup_at = Sys::IRQ::get_curtime() + ((uint64_t) ms) * 1000000;
        currentThread->status    = Thread::state::SLEEPING;

        yield();
    }

    Thread* Thread::getCurrentThread() {
        return Sys::CPU::getCurrentCPU()->currentThread;
    }
}