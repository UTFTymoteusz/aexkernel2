#include "aex/mutex.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

#include "sys/mcore.hpp"

using Thread = AEX::Proc::Thread;

namespace AEX {
    void Mutex::acquire() {
        volatile size_t count = 0;
        Thread::current()->addBusy();

        while (!__sync_bool_compare_and_swap(&m_lock, false, true)) {
            Thread::current()->subBusy();

            count++;
            if (count > 12212222 * 50) {
                int  delta = 0;
                auto name  = Debug::addr2name((void*) this, delta);
                if (!name)
                    name = "no idea";

                Debug::stack_trace(0, (Debug::stack_frame*) ((Thread*) m_thread)->context->rsp);

                kpanic("mutex 0x%p <%s+0x%x> hung (val: %i (held by thread 0x%p), cpu: %i)", this,
                       name, delta, m_lock, m_thread, Sys::CPU::currentID());
            }

            if (Thread::current()->isCritical() && count == 1) {
                int  delta = 0;
                auto name  = Debug::addr2name((void*) this, delta);
                if (!name)
                    name = "no idea";

                printk(PRINTK_WARN "Attempt to acquire 0x%p <%s+0x%x> while critical\n", this, name,
                       delta);

                Debug::stack_trace(0, (Debug::stack_frame*) ((Thread*) m_thread)->context->rsp);
            }

            if (Sys::MCore::cpu_count == 1)
                Proc::Thread::yield();

            Thread::current()->addBusy();
        }

        m_thread = Thread::current();

        Thread::current()->held_mutexes++;
        __sync_synchronize();
    }

    void Mutex::release() {
        __sync_synchronize();

        AEX_ASSERT(Thread::current()->isBusy());
        AEX_ASSERT(__sync_bool_compare_and_swap(&m_lock, true, false));

        m_thread = nullptr;

        Thread::current()->held_mutexes--;
        Thread::current()->subBusy();
    }

    bool Mutex::tryAcquire() {
        Thread::current()->addBusy();

        bool ret = __sync_bool_compare_and_swap(&m_lock, false, true);
        if (!ret)
            Thread::current()->subBusy();
        else {
            m_thread = Thread::current();
            Thread::current()->held_mutexes++;
        }

        __sync_synchronize();

        return ret;
    }

    ScopeMutex Mutex::scope() {
        return ScopeMutex(*this);
    }
}