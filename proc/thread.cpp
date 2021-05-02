#include "aex/proc/thread.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/ipc/event.hpp"
#include "aex/mem.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/proc/broker.hpp"
#include "aex/proc/process.hpp"
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
        this->self        = this;
        this->context     = new Context();
        this->context_aux = new Context();
        this->parent      = process;
        this->status      = TS_FRESH;

        Mem::atomic_add(&process->thread_counter, 1);
    }

    Thread::~Thread() {
        this->lock.acquire();

        delete context;
        delete context_aux;

        if (user_stack)
            parent->pagemap->free((void*) user_stack, user_stack_size);

        if (kernel_stack)
            Mem::kernel_pagemap->free((void*) kernel_stack, kernel_stack_size);

        if (fault_stack)
            Mem::kernel_pagemap->free((void*) fault_stack, fault_stack_size);

        if (aux_stack)
            Mem::kernel_pagemap->free((void*) aux_stack, aux_stack_size);

        Mem::atomic_sub(&parent->thread_counter, 1);

        this->lock.release();
    }

    optional<Thread*> Thread::create(pid_t ppid, void* entry, size_t stack_size,
                                     Mem::Pagemap* pagemap, bool usermode, bool dont_add) {
        processes_lock.acquire();

        auto thread = new Thread();
        auto parent = get_process(ppid);

        parent->threads_lock.acquire();
        processes_lock.release();

        if (!pagemap)
            pagemap = parent->pagemap;

        thread->status = TS_FRESH;
        thread->parent = parent;

        thread->alloc_stacks(pagemap, stack_size, usermode);
        thread->setup_context(pagemap, stack_size, entry, usermode);

        if (parent->tls_size != 0)
            thread->alloc_tls(parent->tls_size);

        thread->original_entry = entry;

        Mem::atomic_add(&parent->thread_counter, 1);

        parent->threads_lock.release();

        parent->assoc(thread);

        if (!dont_add)
            Proc::add_thread(thread);

        return thread;
    }

    void Thread::yield() {
        auto thread = Thread::current();

        AEX_ASSERT(!thread->isCritical());
        AEX_ASSERT(CPU::checkInterrupts());

        if (thread->status == TS_BLOCKED && thread->held_mutexes > 0 && process_list_size > 2) {
            printk(PRINTK_WARN "Thread yielded in state TS_BLOCKED whilst holding a mutex\n");
            Debug::stack_trace();
        }

        CPU::nointerrupts();
        CPU::current()->should_yield = false;

        proc_reshed();

        CPU::interrupts();
    }

    void Thread::sleep(uint64_t ms) {
        usleep(ms * 1000000);
    }

    void Thread::usleep(uint64_t ns) {
        auto current = Thread::current();

        current->wakeup_at = Sys::Time::uptime() + (Sys::Time::time_t) ns;
        current->status    = TS_SLEEPING;

        Thread::yield();

        if (current->interrupted())
            printk("sleep: Interrupted [%i, 0x%p]\n", current->getProcess()->pid, current);
    }

    // TODO: Make this migrate pending signals over
    void Thread::exit() {
        // If we're running we have the lock acquired
        auto thread = Thread::current();

        AEX_ASSERT(!thread->lock.tryAcquire());
        AEX_ASSERT(thread->m_pending_signals.count() == 0);
        AEX_ASSERT(CPU::checkInterrupts());

        if (thread->isBusy()) {
            thread->_abort();
            return;
        }

        thread->addCritical();
        thread->setStatus(TS_DEAD);

        if (thread->m_joiner) {
            thread->m_joiner->lock.acquire();
            thread->m_joiner->setStatus(TS_RUNNABLE);
            thread->m_joiner->lock.release();
        }
        else if (thread->m_detached || Mem::atomic_read(&thread->parent->thread_counter) == 1) {
            thread->finish();
        }

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
        SCOPE(this->lock);

        if (m_detached || m_joiner)
            return EINVAL;

        if (this == Thread::current() || Thread::current()->m_joiner == this)
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

        // TODO: this needs to be improved
        if (Thread::current()->interrupted()) {
            m_joiner = nullptr;

            if (status == TS_DEAD)
                finish();
            else if (Thread::current()->aborting())
                m_detached = true;

            Thread::current()->loadState(state);
            Thread::current()->subBusy();

            return EINTR;
        }

        AEX_ASSERT(!this->m_detached);
        AEX_ASSERT(Thread::current()->status == TS_RUNNABLE);

        finish();

        Thread::current()->loadState(state);
        Thread::current()->subBusy();

        return ENONE;
    }

    error_t Thread::detach() {
        SCOPE(this->lock);

        if (m_detached || m_joiner)
            return EINVAL;

        if (status == TS_DEAD)
            finish();

        m_detached = true;
        return ENONE;
    }

    error_t Thread::abort(bool force) {
        SCOPE(this->lock);

        _abort(force);

        return ENONE;
    }

    void Thread::_abort(bool force) {
        m_aborting = true;

        if (force)
            m_detached = true;

        if (isBusy())
            return;

        if (m_joiner) {
            m_joiner->lock.acquire();
            m_joiner->setStatus(TS_RUNNABLE);
            m_joiner->lock.release();
        }
        else if (m_detached) {
            Thread::current()->addBusy();

            setStatus(TS_DEAD);
            finish();

            Thread::current()->subBusy();
        }
    }

    bool Thread::aborting() {
        return m_aborting;
    }

    bool Thread::interrupted() {
        return m_aborting || (m_pending_signals.count() > 0 && !in_signal);
    }

    void broker_cleanup(Thread* thread) {
        SCOPE(thread->parent->threads_lock);
        delete thread;
    }

    void Thread::finish() {
        Thread::current()->addCritical();

        AEX_ASSERT(this->status == TS_DEAD);

        parent->unassoc(this);
        remove_thread(this);

        if (Mem::atomic_read(&parent->thread_counter) == 1)
            parent->exit(0);

        broker(broker_cleanup, this);

        Thread::current()->subCritical();
    }

    Process* Thread::getProcess() {
        // We need atomicity here
        return this->parent;
    }

    void Thread::take_lock() {
        lock.acquire();
    }

    void Thread::setStatus(status_t status) {
        this->status = status;

        if (status == TS_DEAD)
            AEX_ASSERT(getBusy() <= 1);
    }

    void Thread::subBusy() {
        uint16_t busy = Mem::atomic_sub_fetch(&m_busy, (uint16_t) 1);
        if (busy)
            return;

        if (this == Thread::current() && aborting())
            Thread::exit();

        addCritical();

        if (m_pending_signals.count() != 0) {
            int            last = m_pending_signals.count() - 1;
            IPC::siginfo_t info = m_pending_signals[last];

            m_pending_signals.erase(last);

            subCritical();
            handleSignal(info);
        }
        else
            subCritical();
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

        uint16_t res = Mem::atomic_sub_fetch(&m_critical, (uint16_t) 1);
        Mem::atomic_sub(&m_busy, (uint16_t) 1);

        if (res == 0 && CPU::current()->should_yield)
            Thread::yield();
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

    void Thread::alloc_tls(uint16_t size) {
        auto actual_size = size + sizeof(void*);

        auto tls = parent->pagemap->alloc(actual_size, PAGE_WRITE | PAGE_USER);
        auto tlsk =
            Mem::kernel_pagemap->map(actual_size, parent->pagemap->paddrof(tls), PAGE_WRITE);

        auto tls_self = (size_t) tls + size;

        *((size_t*) ((size_t) tlsk + size)) = tls_self;
        this->tls                           = (void*) tls_self;
    }

    void Thread::alloc_stacks(Mem::Pagemap* pagemap, size_t size, bool usermode) {
        if (usermode) {
            user_stack      = (size_t) pagemap->alloc(size, PAGE_WRITE | PAGE_USER);
            user_stack_size = size;

            kernel_stack      = (size_t) Mem::kernel_pagemap->alloc(KERNEL_STACK_SIZE, PAGE_WRITE);
            kernel_stack_size = KERNEL_STACK_SIZE;
        }
        else {
            user_stack      = 0x0000;
            user_stack_size = 0;

            kernel_stack      = (size_t) Mem::kernel_pagemap->alloc(size, PAGE_WRITE);
            kernel_stack_size = size;
        }

        fault_stack      = (size_t) Mem::kernel_pagemap->alloc(FAULT_STACK_SIZE, PAGE_WRITE);
        fault_stack_size = FAULT_STACK_SIZE;

        aux_stack      = (size_t) Mem::kernel_pagemap->alloc(AUX_STACK_SIZE, PAGE_WRITE);
        aux_stack_size = AUX_STACK_SIZE;
    }

    void Thread::setup_context(Mem::Pagemap* pagemap, size_t size, void* entry, bool usermode) {
        if (usermode)
            context = new Context(entry, (void*) user_stack, size, pagemap, true);
        else
            context = new Context(entry, (void*) kernel_stack, size, pagemap, false, Thread::exit);

        context_aux = new Context();
    }
}