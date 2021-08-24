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
        this->self    = this;
        this->context = new Context();
        this->parent  = process;
        this->status  = TS_FRESH;

        Mem::atomic_add(&process->thread_counter, 1);
    }

    Thread::~Thread() {
        SCOPE(lock);

        delete context;

        if (user_stack.ptr)
            parent->pagemap->free(user_stack.ptr, user_stack.size);

        if (kernel_stack.ptr)
            Mem::kernel_pagemap->free(kernel_stack.ptr, kernel_stack.size);

        if (fault_stack.ptr)
            Mem::kernel_pagemap->free(fault_stack.ptr, fault_stack.size);

        Mem::atomic_sub(&parent->thread_counter, 1);
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
            thread->create_tls();

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
            printk(WARN "Thread yielded in state TS_BLOCKED whilst holding a mutex\n");
            Debug::stack_trace();
        }

        interruptible(false) {
            CPU::current()->should_yield = false;
            proc_reshed();
        }
    }

    void Thread::sleep(uint64_t ms) {
        nsleep(ms * 1000000);
    }

    void Thread::nsleep(uint64_t ns) {
        auto current = Thread::current();

        current->wakeup_at = Sys::Time::uptime() + (Sys::Time::time_t) ns;
        current->status    = TS_SLEEPING;

        Thread::yield();

        if (current->interrupted()) {
            printk("sleep: Interrupted [%i, 0x%p]\n", current->getProcess()->pid, current);
            current->status = TS_RUNNABLE;
        }
    }

    // TODO: Make this migrate pending signals over
    void Thread::exit(void* retval, bool ignoreBusy) {
        // If we're running we have the lock acquired
        auto thread = Thread::current();

        if (thread->m_retval_set) {
            printkd(PTKD_PROC_TH, "proc: th0x%p: Exit %s\n", Thread::current(),
                    ignoreBusy ? "finalized" : "attempted");
        }
        else {
            printkd(PTKD_PROC_TH, "proc: th0x%p: Exit 0x%p\n", Thread::current(), retval);
            thread->retval = retval;
        }

        thread->m_retval_set = true;

        AEX_ASSERT(!thread->lock.tryAcquire());
        // AEX_ASSERT(thread->sigqueue.count() == 0);
        AEX_ASSERT(CPU::checkInterrupts());

        if (thread->isBusy() && !ignoreBusy && !thread->safe_exit) {
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

    optional<void*> Thread::join() {
        if (this == Thread::current()) {
            printkd(PTKD_PROC_TH, "proc: th0x%p: Attempted to join itself\n", Thread::current());
            return EDEADLK;
        }

        SCOPE(this->lock);

        if (m_detached || m_joiner)
            return EINVAL;

        if (Thread::current()->m_joiner == this)
            return EDEADLK;

        printkd(PTKD_PROC_TH, "proc: th0x%p: Joining th0x%p\n", Thread::current(), this);

        // I've forgotten about joining an already-exitted thread.
        if (status == TS_DEAD) {
            auto retval = this->retval;
            finish();

            return retval;
        }

        auto state = Thread::current()->saveState();

        m_joiner = Thread::current();
        Thread::current()->setStatus(TS_BLOCKED);

        this->lock.release();
        Thread::yield();
        this->lock.acquire();

        // TODO: this needs to be improved
        if (Thread::current()->aborting()) {
            m_joiner = nullptr;

            if (status == TS_DEAD)
                finish();
            else if (Thread::current()->aborting())
                m_detached = true;

            Thread::current()->loadState(state);
            return EINTR;
        }

        if (Thread::current()->interrupted()) {
            m_joiner = nullptr;

            Thread::current()->loadState(state);
            return EINTR;
        }

        AEX_ASSERT(!this->m_detached);
        AEX_ASSERT(Thread::current()->status == TS_RUNNABLE);

        auto retval = this->retval;
        finish();

        Thread::current()->loadState(state);

        return retval;
    }

    error_t Thread::detach() {
        SCOPE(this->lock);

        if (m_detached || m_joiner)
            return EINVAL;

        if (status == TS_DEAD)
            finish();

        printkd(PTKD_PROC_TH, "proc: th0x%p: Detached th0x%p\n", Thread::current(), this);
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

        if (isBusy() && !safe_exit)
            return;

        if (m_joiner) {
            m_joiner->lock.acquire();
            m_joiner->setStatus(TS_RUNNABLE);
            m_joiner->lock.release();
        }
        else if (m_detached) {
            setStatus(TS_DEAD);
            finish();
        }
    }

    bool Thread::aborting() {
        return m_aborting;
    }

    bool Thread::interrupted() {
        return (m_aborting ||
                (parent->m_sigqueue.countUnmasked() > 0 || m_sigqueue.countUnmasked() > 0)) &&
               m_signability > 0;
    }

    void broker_cleanup(Thread_SP* thread_sp_ptr) {
        auto thread = *thread_sp_ptr;
        delete thread_sp_ptr;

        while (thread.refCount() > 1)
            Thread::sleep(2);

        SCOPE(thread->parent->threads_lock);

        delete thread.get();
        thread.defuse();
    }

    void Thread::finish() {
        SCOPE(Thread::current()->criticalGuard);
        AEX_ASSERT(this->status == TS_DEAD);

        auto thread = parent->unassoc(this);
        if (!thread)
            thread = Thread_SP(this);

        remove_thread(this);

        if (Mem::atomic_read(&parent->thread_counter) == 1)
            parent->exit(0);

        broker(broker_cleanup, new Thread_SP(thread));
    }

    void Thread::abortCheck() {
        if (Thread::current()->m_aborting)
            Thread::exit((void*) 0x2137, true);
    }

    Process* Thread::getProcess() {
        // We need atomicity here
        return this->parent;
    }

    void Thread::setStatus(status_t status) {
        this->status = status;

        // if (status == TS_DEAD)
        //    AEX_ASSERT(!isBusy());
    }

    bool Thread::isBusy() {
        AEX_ASSERT(Thread::current()->lock.isAcquired());

        return (Thread::current() == this && !Sys::CPU::current()->rescheduling) ||
               context->kernelmode();
    }

    void Thread::addSignability() {
        Mem::atomic_add(&m_signability, (uint16_t) 1);
    }

    void Thread::subSignability() {
        Mem::atomic_sub(&m_signability, (uint16_t) 1);
    }

    void Thread::addCritical() {
        Mem::atomic_add(&m_critical, (uint16_t) 1);

        // if (!CPU::current()->in_interrupt)
        //    CPU::nointerrupts();
    }

    void Thread::subCritical() {
        // if (Mem::atomic_sub_fetch(&m_critical, (uint16_t) 1) == 0 &&
        // !CPU::current()->in_interrupt)
        //    CPU::interrupts();

        uint16_t res = Mem::atomic_sub_fetch(&m_critical, (uint16_t) 1);
        if (res == 0 && CPU::current()->should_yield)
            Thread::yield();
    }

    Thread::state Thread::saveState() {
        auto st = state();

        st.signability = m_signability;
        st.critical    = m_critical;
        st.status      = status;

        return st;
    }

    void Thread::loadState(state& m_state) {
        m_signability = m_state.signability;
        m_critical    = m_state.critical;
        status        = m_state.status;
    }

    void Thread::create_tls() {
        if (parent->tls_size == 0)
            return;

        auto actual_size = parent->tls_size + sizeof(void*);

        auto tls = parent->pagemap->alloc(actual_size, PAGE_WRITE | PAGE_USER);
        auto tlsk =
            Mem::kernel_pagemap->map(actual_size, parent->pagemap->paddrof(tls), PAGE_WRITE);

        auto tls_self = (size_t) tls + parent->tls_size;

        *((size_t*) ((size_t) tlsk + parent->tls_size)) = tls_self;
        this->tls                                       = (void*) tls_self;

        memcpy((char*) tlsk, parent->tls_base, parent->tls_size);

        Mem::kernel_pagemap->free(tlsk, actual_size);
    }

    void Thread::alloc_stacks(Mem::Pagemap* pagemap, size_t size, bool usermode) {
        if (usermode) {
            user_stack.ptr  = pagemap->alloc(size, PAGE_WRITE | PAGE_USER);
            user_stack.size = size;

            kernel_stack.ptr  = Mem::kernel_pagemap->alloc(KERNEL_STACK_SIZE, PAGE_WRITE);
            kernel_stack.size = KERNEL_STACK_SIZE;
        }
        else {
            user_stack.ptr  = nullptr;
            user_stack.size = 0;

            kernel_stack.ptr  = Mem::kernel_pagemap->alloc(size, PAGE_WRITE);
            kernel_stack.size = size;
        }

        fault_stack.ptr  = Mem::kernel_pagemap->alloc(FAULT_STACK_SIZE, PAGE_WRITE);
        fault_stack.size = FAULT_STACK_SIZE;
    }

    void Thread::setup_context(Mem::Pagemap* pagemap, size_t size, void* entry, bool usermode) {
        if (usermode)
            context = new Context(entry, user_stack.ptr, size, pagemap, true);
        else
            context =
                new Context(entry, kernel_stack.ptr, size, pagemap, false, Thread::exit_implicit);
    }
}