#include "aex/spinlock.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

#include "sys/mcore.hpp"

using Thread = AEX::Proc::Thread;

namespace AEX {
    bool spinlock_faulted = false;

    inline bool acquireLock(volatile int* lock) {
        return __sync_bool_compare_and_swap(lock, false, true);
    }

    inline bool releaseLock(volatile int* lock) {
        return __sync_bool_compare_and_swap(lock, true, false);
    }

    void Spinlock::acquire() {
        volatile size_t count = 0;
        Thread::current()->addCritical();

        while (!acquireLock(&m_lock)) {
            Thread::current()->subCritical();

            asm volatile("pause");
            count++;

            if (count == 40213259) {
                if (Thread::current() == m_thread)
                    fail("recursive");

                fail("hung");
            }

            if (Sys::MCore::cpu_count == 1 && !Thread::current()->isCritical())
                Proc::Thread::yield();

            Thread::current()->addCritical();
        }

        acquired();
        __sync_synchronize();
    }

    void Spinlock::release() {
        __sync_synchronize();

        if (!releaseLock(&m_lock)) {
            Sys::CPU::nointerrupts();

            printk_fault();
            fail("too many releases");
        }

        __sync_synchronize();
        Thread::current()->subCritical();

        released();
    }

    bool Spinlock::tryAcquire() {
        Thread::current()->addCritical();

        bool ret = __sync_bool_compare_and_swap(&m_lock, false, true);
        if (!ret)
            Thread::current()->subCritical();
        else
            acquired();

        __sync_synchronize();

        return ret;
    }

    bool Spinlock::isAcquired() {
        return m_lock;
    }

    void Spinlock::acquireRaw() {
        volatile size_t count = 0;

        while (!acquireLock(&m_lock)) {
            asm volatile("pause");
            count++;

            if (count > 12212222 * 10) {
                if (__sync_bool_compare_and_swap(&spinlock_faulted, false, true)) {
                    fail("hung raw");
                }
                else
                    Sys::CPU::halt();
            }
        }

        acquired(true);
        __sync_synchronize();
    }

    void Spinlock::releaseRaw() {
        __sync_synchronize();

        if (!releaseLock(&m_lock)) {
            Sys::CPU::nointerrupts();

            printk_fault();
            fail("too many releases raw");
        }

        released(true);
    }

    bool Spinlock::tryAcquireRaw() {
        bool ret = acquireLock(&m_lock);
        if (ret)
            acquired(true);

        __sync_synchronize();
        return ret;
    }

    bool Spinlock::tryReleaseRaw() {
        __sync_synchronize();
        released(true);

        return releaseLock(&m_lock);
    }

    void Spinlock::trace() {
#ifdef DEBUG
        printk("Stack trace at acquisition:\n");

        for (int i = 0; i < TRACE_DEPTH; i++) {
            void* caller = callers[i];
            int   delta  = 0;
            auto  name   = Debug::addr2name(caller, delta);

            printk("%p <%s+0x%x>\n", caller, name ?: "no idea", delta);
        }
#endif
    }

    ScopeSpinlock Spinlock::scope() {
        return ScopeSpinlock(*this);
    }

    void Spinlock::acquired(bool raw) {
        m_thread = raw ? nullptr : Thread::current();

#ifdef DEBUG
        for (int i = 0; i < TRACE_DEPTH; i++)
            callers[i] = Debug::caller(i);
#endif
    }

    void Spinlock::released(bool raw) {
        m_thread = raw ? nullptr : Thread::current();

#ifdef DEBUG
        for (int i = 0; i < TRACE_DEPTH; i++)
            callers[i] = Debug::caller(i);
#endif
    }

    void Spinlock::fail(const char* reason) {
        int  delta = 0;
        auto name  = Debug::addr2name((void*) this, delta) ?: "no idea";

        if (__sync_bool_compare_and_swap(&spinlock_faulted, false, true)) {
            printk(FAIL "spinlock %p <%s+0x%x> fail (%s) (val: %i (held by thread %p), cpu: %i)\n",
                   this, name, delta, reason, m_lock, m_thread, Sys::CPU::currentID());

#ifdef DEBUG
            printk("Stack trace at acquisition:\n");

            for (int i = 0; i < TRACE_DEPTH; i++) {
                void* caller = callers[i];
                int   delta  = 0;
                auto  name   = Debug::addr2name(caller, delta);

                printk("%p <%s+0x%x>\n", caller, name ?: "no idea", delta);
            }
#endif

            kpanic("spinlock fail");
        }
        else {
            Sys::CPU::halt();
        }
    }
}