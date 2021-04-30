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

            if (count > 12212222 * 50) {
                int  delta = 0;
                auto name  = Debug::addr2name((void*) this, delta);
                if (!name)
                    name = "no idea";

                if (__sync_bool_compare_and_swap(&spinlock_faulted, false, true))
                    kpanic("spinlock 0x%p <%s+0x%x> hung (val: %i (held by thread 0x%p), cpu: %i)",
                           this, name, delta, m_lock, m_thread, Sys::CPU::currentID());
                else
                    Sys::CPU::halt();
            }

            if (Sys::MCore::cpu_count == 1 && !Thread::current()->isCritical())
                Proc::Thread::yield();

            Thread::current()->addCritical();
        }

        m_thread = Thread::current();
        __sync_synchronize();
    }

    void Spinlock::release() {
        __sync_synchronize();

        if (!releaseLock(&m_lock)) {
            Sys::CPU::nointerrupts();

            printk_fault();

            int  delta = 0;
            auto name  = Debug::addr2name((void*) &m_lock, delta);
            if (!name)
                name = "no idea";

            printk("aaa (0x%p, <%s>+0x%x)\n", this, name, delta);
            Debug::stack_trace();
            kpanic("spinlock: Too many releases");
        }

        __sync_synchronize();
        Thread::current()->subCritical();

        m_thread = nullptr;
    }

    bool Spinlock::tryAcquire() {
        Thread::current()->addCritical();

        bool ret = __sync_bool_compare_and_swap(&m_lock, false, true);
        if (!ret)
            Thread::current()->subCritical();

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
                int  delta = 0;
                auto name  = Debug::addr2name((void*) this, delta);
                if (!name)
                    name = "no idea";

                if (__sync_bool_compare_and_swap(&spinlock_faulted, false, true)) {
                    kpanic("spinlock 0x%p <%s+0x%x> hung (val: %i, cpu: %i) *RAW*", this, name,
                           delta, m_lock, Sys::CPU::currentID());
                }
                else
                    Sys::CPU::halt();
            }
        }

        __sync_synchronize();
    }

    void Spinlock::releaseRaw() {
        __sync_synchronize();

        if (!releaseLock(&m_lock)) {
            Sys::CPU::nointerrupts();

            printk_fault();

            int  delta = 0;
            auto name  = Debug::addr2name((void*) &m_lock, delta);
            if (!name)
                name = "no idea";

            printk("bbb (0x%p, <%s>+0x%x), %i\n", this, name, delta, m_lock);
            Debug::stack_trace();
            kpanic("spinlock: Too many releases");
        }
    }

    bool Spinlock::tryAcquireRaw() {
        bool ret = acquireLock(&m_lock);

        __sync_synchronize();
        return ret;
    }

    bool Spinlock::tryReleaseRaw() {
        __sync_synchronize();
        return releaseLock(&m_lock);
    }

    ScopeSpinlock Spinlock::scope() {
        return ScopeSpinlock(*this);
    }
}