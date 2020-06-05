#include "aex/proc/thread.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/ipc/event.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"

#include "proc/context.hpp"
#include "proc/proc.hpp"
#include "sys/irq.hpp"

#include <stddef.h>

namespace AEX::Proc {
    extern "C" void proc_reshed();

    Thread::Thread() {
        context           = new Context();
        this->_exit_event = new IPC::Event();
    }

    Thread::Thread(Process* parent) {
        status  = FRESH;
        context = new Context();

        this->parent      = parent;
        this->_exit_event = new IPC::Event();
    }

    Thread::Thread(Process* parent, void* entry, size_t stack_size, VMem::Pagemap* pagemap,
                   bool usermode, bool dont_add) {
        if (!parent)
            parent = processes.get(1).get();

        if (!pagemap)
            pagemap = parent->pagemap;

        _stack = pagemap->alloc(stack_size, PAGE_WRITE);

        if (usermode)
            context = new Context(entry, _stack, stack_size, pagemap, usermode);
        else
            context = new Context(entry, _stack, stack_size, pagemap, usermode, Thread::exit);

        status = FRESH;

        this->parent      = parent;
        this->_exit_event = new IPC::Event();

        if (dont_add)
            return;

        Proc::add_thread(this);

        parent->lock.acquire();

        this->refs->increment();
        parent->threads.addRef(this, this->refs);

        parent->lock.release();
    }

    Thread::~Thread() {
        delete refs;
        delete _exit_event;

        delete context;

        // unalloc da stack pls
        // if (stack)
        //    parent->pagemap->;

        // printk("thread cleaned\n");
    }

    void Thread::yield() {
        if (Thread::getCurrentThread()->isCritical())
            return;

        Sys::CPU::getCurrentCPU()->willingly_yielded = true;
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

    bool Thread::shouldExit() {
        return getCurrentThread()->isAbortSet();
    }

    void Thread::exit() {
        auto thread = Thread::getCurrentThread();

        if (thread->isBusy())
            kpanic("Attempt to exit a thread while it's still busy\n");

        Mem::atomic_compare_and_swap(&thread->_finished, (uint8_t) 0, (uint8_t) 1);
        Mem::atomic_compare_and_swap(&thread->_abort, (uint8_t) 0, (uint8_t) 1);

        abort_thread(thread);

        while (true)
            ;
    }

    Mem::SmartPointer<Process> Thread::getProcess() {
        return processes.get(this->parent->pid);
    }

    void Thread::start() {
        if (status == FRESH)
            status = RUNNABLE;
    }

    void Thread::join() {
        auto sp = this->getSmartPointer();
        sp->_exit_event->wait();
    }

    void Thread::abort(bool block) {
        auto sp = this->getSmartPointer();
        sp->lock.acquire();

        Mem::atomic_compare_and_swap(&_abort, (uint8_t) 0, (uint8_t) 1);

        if (!sp->isBusy()) {
            Mem::atomic_compare_and_swap(&sp->_finished, (uint8_t) 0, (uint8_t) 1);

            sp->lock.release();
            abort_thread(sp.get());

            return;
        }

        sp->lock.release();

        if (block)
            sp->join();
    }

    bool Thread::isAbortSet() {
        return Mem::atomic_read(&_abort) > 0;
    }

    bool Thread::isFinished() {
        return Mem::atomic_read(&_finished) > 0;
    }

    Mem::SmartPointer<Thread> Thread::getSmartPointer() {
        this->refs->increment();
        return Mem::SmartPointer<Thread>(this, this->refs);
    }

    void Thread::setStatus(status_t status) {
        this->status = status;
    }

    void Thread::announceExit() {
        _exit_event->defunct();
    }

    void Thread::subCritical() {
        if (Mem::atomic_sub_fetch(&_critical, (uint16_t) 1) == 0 &&
            Sys::CPU::getCurrentCPU()->should_yield) {
            Mem::atomic_sub(&_busy, (uint16_t) 1);

            Proc::Thread::yield();
            return;
        }

        Mem::atomic_sub(&_busy, (uint16_t) 1);
    }
}