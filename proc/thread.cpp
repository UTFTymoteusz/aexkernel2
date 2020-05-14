#include "aex/proc/thread.hpp"

#include "aex/mem/smartptr.hpp"
#include "aex/printk.hpp"

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
        if (usermode)
            context = Context(entry, stack, stack_size, pagemap, usermode);
        else
            context = Context(entry, stack, stack_size, pagemap, usermode, _kthread_exit);

        status = FRESH;

        this->parent = parent;

        if (dont_add)
            return;

        Proc::add_thread(this);

        parent->lock.acquire();
        parent->threads.addRef(this, this->refs);

        this->refs->increment();

        parent->lock.release();
    }

    Thread::~Thread() {
        delete refs;
        // cleanup the context too pls
    }

    void Thread::start() {
        if (status == FRESH)
            status = RUNNABLE;
    }

    Mem::SmartPointer<Thread> Thread::getSmartPointer() {
        this->refs->increment();
        return Mem::SmartPointer<Thread>(this, this->refs);
    }

    void Thread::setStatus(status_t status) {
        this->status = status;
    }

    Mem::SmartPointer<Process> Thread::getProcess() {
        return processes.get(this->parent->pid);
    }

    void Thread::yield() {
        if (Thread::getCurrentThread()->isCritical())
            return;

        proc_reshed();
    }

    void Thread::sleep(int ms) {
        auto currentThread = Thread::getCurrentThread();

        currentThread->wakeup_at = Sys::IRQ::get_uptime() + ((uint64_t) ms) * 1000000;
        currentThread->status    = Thread::status_t::SLEEPING;

        yield();
    }

    Thread* Thread::getCurrentThread() {
        return Sys::CPU::getCurrentCPU()->currentThread;
    }

    tid_t Thread::getCurrentTID() {
        return getCurrentThread()->tid;
    }
}