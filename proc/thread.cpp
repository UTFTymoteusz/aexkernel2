#include "aex/proc/thread.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/ipc/event.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/proc/broker.hpp"
#include "aex/sys/time.hpp"

#include "proc/proc.hpp"

using CPU = AEX::Sys::CPU;

namespace AEX::Proc {
    extern "C" void proc_reshed();

    // create &
    // start  &
    // join   &
    // detach &
    // yield  &
    // sleep  &
    // abort  &?
    // exit   &

    Thread::Thread() {
        this->self   = this;
        this->status = TS_FRESH;
    }

    Thread::Thread(Process* process) {
        this->self    = this;
        this->context = new Context();
        this->parent  = process;
        this->status  = TS_FRESH;
    }

    Thread::~Thread() {
        this->lock.acquire();

        delete context;

        if (user_stack)
            parent->pagemap->free((void*) user_stack, user_stack_size);

        if (kernel_stack)
            parent->pagemap->free((void*) kernel_stack, kernel_stack_size);

        if (fault_stack)
            parent->pagemap->free((void*) fault_stack, fault_stack_size);

        this->lock.release();
    }

    optional<Thread*> Thread::create(Process* parent, void* entry, size_t stack_size,
                                     Mem::Pagemap* pagemap, bool usermode, bool dont_add) {
        auto thread = new Thread();

        if (!parent)
            parent = get_process(1);

        if (!pagemap)
            pagemap = parent->pagemap;

        if (usermode) {
            thread->user_stack      = (size_t) pagemap->alloc(stack_size, PAGE_WRITE);
            thread->user_stack_size = stack_size;

            thread->kernel_stack =
                (size_t) Mem::kernel_pagemap->alloc(KERNEL_STACK_SIZE, PAGE_WRITE);
            thread->kernel_stack_size = KERNEL_STACK_SIZE;

            thread->context =
                new Context(entry, (void*) thread->user_stack, stack_size, pagemap, true);
        }
        else {
            thread->user_stack      = 0;
            thread->user_stack_size = 0;

            thread->kernel_stack      = (size_t) Mem::kernel_pagemap->alloc(stack_size, PAGE_WRITE);
            thread->kernel_stack_size = stack_size;

            thread->context = new Context(entry, (void*) thread->kernel_stack, stack_size, pagemap,
                                          false, Thread::exit);
        }

        thread->fault_stack      = (size_t) pagemap->alloc(FAULT_STACK_SIZE, PAGE_WRITE);
        thread->fault_stack_size = FAULT_STACK_SIZE;

        thread->status = TS_RUNNABLE;
        thread->parent = parent;

        thread->original_entry = entry;

        parent->lock.acquire();
        parent->threads.pushBack(thread);
        parent->lock.release();

        if (!dont_add)
            Proc::add_thread(thread);

        return thread;
    }

    void Thread::yield() {
        bool ints = CPU::checkInterrupts();
        CPU::nointerrupts();

        proc_reshed();

        if (ints)
            CPU::interrupts();
    }

    void Thread::sleep(int ms) {
        auto currentThread = Thread::current();

        currentThread->wakeup_at = Sys::Time::uptime() + ((Sys::Time::time_t) ms) * 1000000;
        currentThread->status    = TS_SLEEPING;

        Thread::yield();
    }

    void Thread::exit() {
        // If we're running we have the lock acquired
        auto thread = Thread::current();

        AEX_ASSERT(!thread->lock.tryAcquire());
        AEX_ASSERT(!thread->isBusy());
        AEX_ASSERT(CPU::checkInterrupts());

        thread->addCritical();
        thread->setStatus(TS_DEAD);

        if (thread->m_joiner) {
            thread->m_joiner->lock.acquire();
            thread->m_joiner->setStatus(TS_RUNNABLE);
            thread->m_joiner->lock.release();
        }

        if (thread->m_detached)
            thread->finish();

        thread->subCritical();
        Thread::yield();

        while (true)
            CPU::wait();
    }

    Thread* Thread::current() {
        // We need atomicity here
        return CPU::currentThread();
    }

    error_t Thread::start() {
        if (status == TS_FRESH)
            status = TS_RUNNABLE;

        return ENONE;
    }

    error_t Thread::join() {
        auto scope = this->lock.scope();

        if (m_detached || m_joiner)
            return EINVAL;

        if (this == Thread::current())
            return EDEADLK;

        Thread::current()->addBusy();

        // I've forgotten about joining an already-exitted thread.
        if (status == TS_DEAD) {
            finish();
            Thread::current()->subBusy();

            return ENONE;
        }

        auto state = Thread::current()->saveState();

        m_joiner = Thread::current();
        Thread::current()->setStatus(TS_BLOCKED);

        this->lock.release();
        Thread::yield();
        this->lock.acquire();

        if (Thread::current()->aborting()) {
            printk("we hath been interrupted\n");

            m_detached = true;
            m_joiner   = nullptr;

            Thread::current()->loadState(state);
            Thread::current()->subBusy();

            return EINTR;
        }

        AEX_ASSERT(Thread::current()->status == TS_RUNNABLE);

        finish();

        Thread::current()->loadState(state);
        Thread::current()->subBusy();

        return ENONE;
    }

    error_t Thread::detach() {
        auto scope = this->lock.scope();

        if (m_detached || m_joiner)
            return EINVAL;

        if (status == TS_DEAD)
            finish();

        m_detached = true;
        return ENONE;
    }

    error_t Thread::abort() {
        auto scope = this->lock.scope();

        m_aborting = true;

        if (!isBusy()) {
            if (m_detached) {
                setStatus(TS_DEAD);
                finish();
            }
            else if (m_joiner) {
                m_joiner->lock.acquire();
                m_joiner->setStatus(TS_RUNNABLE);
                m_joiner->lock.release();
            }
        }

        return ENONE;
    }

    bool Thread::aborting() {
        return m_aborting;
    }

    void broker_cleanup(Thread* thread) {
        delete thread;
    }

    void Thread::finish() {
        parent->lock.acquire();

        for (int i = 0; i < parent->threads.count(); i++) {
            if (parent->threads[i] != this)
                continue;

            parent->threads.erase(i);
            break;
        }

        parent->lock.release();

        if (parent->threads.realCount() == 0)
            parent->exit(0);

        remove_thread(this);
        broker(broker_cleanup, this);
    }

    Process* Thread::getProcess() {
        // We need atomicity here
        return this->parent;
    }

    void Thread::setStatus(thread_status_t status) {
        this->status = status;
    }

    void Thread::addCritical() {
        Mem::atomic_add(&m_busy, (uint16_t) 1);
        Mem::atomic_add(&m_critical, (uint16_t) 1);

        // if (!CPU::current()->in_interrupt)
        //    CPU::nointerrupts();
    }

    void Thread::subCritical() {
        // if (Mem::atomic_sub_fetch(&m_critical, (uint16_t) 1) == 0 &&
        // !CPU::current()->in_interrupt)
        //    CPU::interrupts();

        Mem::atomic_sub(&m_critical, (uint16_t) 1);
        Mem::atomic_sub(&m_busy, (uint16_t) 1);
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