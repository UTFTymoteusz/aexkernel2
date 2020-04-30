#include "proc/thread.hpp"

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
                   VMem::Pagemap* pagemap) {
        context = Context(entry, stack, stack_size, pagemap);
        status  = FRESH;

        this->parent = parent;
    }

    void Thread::start() {
        Proc::add_thread(this);

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

        currentThread->wakeup_at = (size_t) Sys::IRQ::get_curtime() + ms;
        currentThread->status    = Thread::state::SLEEPING;

        yield();
    }

    Thread* Thread::getCurrentThread() {
        return Sys::CPU::getCurrentCPU()->currentThread;
    }
}