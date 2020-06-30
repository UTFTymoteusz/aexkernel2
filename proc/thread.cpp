#include "aex/proc/thread.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/ipc/event.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/sys/time.hpp"

#include "proc/context.hpp"
#include "proc/proc.hpp"

using CPU = AEX::Sys::CPU;

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

    Thread::Thread(Process* parent, void* entry, size_t stack_size, Mem::Pagemap* pagemap,
                   bool usermode, bool dont_add) {
        if (!parent)
            parent = processes.get(1).get();

        if (!pagemap)
            pagemap = parent->pagemap;

        _stack      = pagemap->alloc(stack_size, PAGE_WRITE);
        _stack_size = stack_size;

        if (usermode)
            context = new Context(entry, _stack, stack_size, pagemap, true);
        else
            context = new Context(entry, _stack, stack_size, pagemap, false, Thread::exit);

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

        if (_stack)
            parent->pagemap->free(_stack, _stack_size);
    }

    void Thread::yield() {
        if (Thread::getCurrentThread()->isCritical())
            return;

        if (CPU::getCurrentCPU()->in_interrupt)
            kpanic("attempt to yield() while in interrupts");

        CPU::getCurrentCPU()->willingly_yielded = true;
        proc_reshed();
    }

    void Thread::sleep(int ms) {
        auto currentThread = Thread::getCurrentThread();

        currentThread->wakeup_at = Sys::get_uptime() + ((uint64_t) ms) * 1000000;
        currentThread->status    = Thread::status_t::SLEEPING;

        yield();
    }

    Thread* Thread::getCurrentThread() {
        return CPU::getCurrentCPU()->currentThread;
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

        // A while(true) is boring

        volatile size_t a = 0;
        volatile size_t b = 1;
        volatile size_t c;

        while (true) {
            c = a + b;

            a = b;
            b = c;
        }
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

    void Thread::addCritical() {
        if (!CPU::getCurrentCPU()->in_interrupt)
            CPU::nointerrupts();

        Mem::atomic_add(&_busy, (uint16_t) 1);
        Mem::atomic_add(&_critical, (uint16_t) 1);

        // printk("interrupts are gone\n");
    }

    void Thread::subCritical() {
        uint16_t fetched = Mem::atomic_sub_fetch(&_critical, (uint16_t) 1);

        if (fetched == 0 && CPU::getCurrentCPU()->should_yield) {
            Mem::atomic_sub(&_busy, (uint16_t) 1);

            if (!CPU::getCurrentCPU()->in_interrupt)
                CPU::interrupts();

            Proc::Thread::yield();
            return;
        }

        Mem::atomic_sub(&_busy, (uint16_t) 1);

        if (fetched == 0 && !CPU::getCurrentCPU()->in_interrupt)
            CPU::interrupts();
    }
}