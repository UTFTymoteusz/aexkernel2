#include "aex/proc/thread.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/ipc/event.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/sys/time.hpp"

#include "proc/proc.hpp"

using CPU = AEX::Sys::CPU;

namespace AEX::Proc {
    extern "C" void proc_reshed();

    Thread::Thread() {
        this->self = this;

        this->context      = new Context();
        this->m_exit_event = new IPC::Event();
    }

    Thread::Thread(Process* parent) {
        this->self = this;

        status  = TS_FRESH;
        context = new Context();

        this->parent       = parent;
        this->m_exit_event = new IPC::Event();
    }

    Thread::Thread(Process* parent, void* entry, size_t stack_size, Mem::Pagemap* pagemap,
                   bool usermode, bool dont_add) {
        this->self = this;

        if (!parent)
            parent = processes.get(1).get();

        if (!pagemap)
            pagemap = parent->pagemap;

        if (usermode) {
            user_stack      = (size_t) pagemap->alloc(stack_size, PAGE_WRITE);
            user_stack_size = stack_size;

            kernel_stack      = (size_t) Mem::kernel_pagemap->alloc(KERNEL_STACK_SIZE, PAGE_WRITE);
            kernel_stack_size = KERNEL_STACK_SIZE;

            context = new Context(entry, (void*) user_stack, stack_size, pagemap, true);
        }
        else {
            user_stack      = 0;
            user_stack_size = 0;

            kernel_stack      = (size_t) Mem::kernel_pagemap->alloc(stack_size, PAGE_WRITE);
            kernel_stack_size = stack_size;

            context =
                new Context(entry, (void*) kernel_stack, stack_size, pagemap, false, Thread::exit);
        }

        fault_stack      = (size_t) pagemap->alloc(FAULT_STACK_SIZE, PAGE_WRITE);
        fault_stack_size = FAULT_STACK_SIZE;

        status = TS_FRESH;

        this->parent       = parent;
        this->m_exit_event = new IPC::Event();

        if (dont_add)
            return;

        Proc::add_thread(this);

        parent->lock.acquire();

        this->shared->increment();
        parent->threads.addRef(this, this->shared);

        parent->lock.release();
    }

    Thread::~Thread() {
        delete shared;
        delete m_exit_event;

        delete context;

        if (user_stack)
            parent->pagemap->free((void*) user_stack, user_stack_size);

        if (kernel_stack)
            parent->pagemap->free((void*) kernel_stack, kernel_stack_size);

        if (fault_stack)
            parent->pagemap->free((void*) fault_stack, fault_stack_size);
    }

    void Thread::yield() {
        bool ints = CPU::checkInterrupts();
        CPU::nointerrupts();

        // CPU::getCurrent()->willingly_yielded = true;
        proc_reshed();

        if (ints)
            CPU::interrupts();
    }

    void Thread::sleep(int ms) {
        auto currentThread = Thread::getCurrent();

        currentThread->wakeup_at = Sys::Time::uptime() + ((Sys::Time::time_t) ms) * 1000000;
        currentThread->status    = TS_SLEEPING;

        yield();
    }

    Thread* Thread::getCurrent() {
        // We need atomicity here
        return CPU::currentThread();
    }

    tid_t Thread::getCurrentTID() {
        return getCurrent()->tid;
    }

    bool Thread::shouldExit() {
        return getCurrent()->isAbortSet();
    }

    void Thread::exit() {
        auto thread = Thread::getCurrent();
        AEX_ASSERT(!thread->isBusy());

        Mem::atomic_compare_and_swap(&thread->m_finished, (uint8_t) 0, (uint8_t) 1);
        Mem::atomic_compare_and_swap(&thread->m_abort, (uint8_t) 0, (uint8_t) 1);

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
        if (status == TS_FRESH)
            status = TS_RUNNABLE;
    }

    void Thread::join() {
        auto sp = this->getSmartPointer();
        sp->m_exit_event->wait();
    }

    void Thread::abort(bool block) {
        auto sp = this->getSmartPointer();
        sp->lock.acquire();

        Mem::atomic_compare_and_swap(&m_abort, (uint8_t) 0, (uint8_t) 1);

        if (!sp->isBusy()) {
            Mem::atomic_compare_and_swap(&sp->m_finished, (uint8_t) 0, (uint8_t) 1);

            sp->lock.release();
            abort_thread(sp.get());

            return;
        }

        sp->lock.release();

        if (block)
            sp->join();
    }

    bool Thread::isAbortSet() {
        return Mem::atomic_read(&m_abort) > 0;
    }

    bool Thread::isFinished() {
        return Mem::atomic_read(&m_finished) > 0;
    }

    Mem::SmartPointer<Thread> Thread::getSmartPointer() {
        this->shared->increment();
        return Mem::SmartPointer<Thread>(this, this->shared);
    }

    void Thread::setStatus(thread_status_t status) {
        this->status = status;
    }

    void Thread::announceExit() {
        m_exit_event->defunct();
    }

    void Thread::addCritical() {
        // if (!CPU::current()->in_interrupt)
        //    CPU::nointerrupts();

        Mem::atomic_add(&m_busy, (uint16_t) 1);
        Mem::atomic_add(&m_critical, (uint16_t) 1);
    }

    void Thread::subCritical() {
        // uint16_t fetched = Mem::atomic_sub_fetch(&m_critical, (uint16_t) 1);

        /*if (fetched == 0 && CPU::getCurrent()->should_yield) {
            Mem::atomic_sub(&m_busy, (uint16_t) 1);

            if (!CPU::getCurrent()->in_interrupt)
                CPU::interrupts();

            Proc::Thread::yield();
            return;
        }*/

        Mem::atomic_sub(&m_critical, (uint16_t) 1);
        Mem::atomic_sub(&m_busy, (uint16_t) 1);

        // if (fetched == 0 && !CPU::current()->in_interrupt)
        //    CPU::interrupts();
    }

    Thread::state Thread::saveState() {
        auto st = state();

        st.busy     = m_busy;
        st.critical = m_critical;
        st.status   = status;

        return st;
    }

    void Thread::loadState(state& m_state) {
        m_busy     = m_state.busy;
        m_critical = m_state.critical;
        status     = m_state.status;
    }
}