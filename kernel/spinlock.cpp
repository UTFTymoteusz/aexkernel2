#include "aex/spinlock.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

using Thread = AEX::Proc::Thread;

namespace AEX {
    void Spinlock::acquire() {
        volatile size_t count = 0;
        Thread::getCurrent()->addCritical();

        while (!__sync_bool_compare_and_swap(&m_lock, false, true)) {
            Thread::getCurrent()->subCritical();

            asm volatile("pause");
            count++;

            if (count > 12212222) {
                int  delta = 0;
                auto name  = Debug::addr2name((void*) this, delta);
                if (!name)
                    name = "no idea";

                kpanic("spinlock 0x%p <%s+0x%x> hung (val: %i, cpu: %i)\n", this, name, delta,
                       m_lock, Sys::CPU::getCurrentID());
            }

            Thread::getCurrent()->addCritical();
        }

        __sync_synchronize();
    }

    void Spinlock::release() {
        __sync_synchronize();

        if (!__sync_bool_compare_and_swap(&m_lock, true, false)) {
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

        Thread::getCurrent()->subCritical();
    }

    bool Spinlock::tryAcquire() {
        Thread::getCurrent()->addCritical();

        bool ret = __sync_bool_compare_and_swap(&m_lock, false, true);
        if (!ret)
            Thread::getCurrent()->subCritical();

        __sync_synchronize();

        return ret;
    }


    void Spinlock::acquireRaw() {
        while (!__sync_bool_compare_and_swap(&m_lock, false, true))
            asm volatile("pause");

        __sync_synchronize();
    }

    void Spinlock::releaseRaw() {
        __sync_synchronize();

        if (!__sync_bool_compare_and_swap(&m_lock, true, false)) {
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
        bool ret = __sync_bool_compare_and_swap(&m_lock, false, true);

        __sync_synchronize();
        return ret;
    }

    bool Spinlock::tryReleaseRaw() {
        __sync_synchronize();

        return __sync_bool_compare_and_swap(&m_lock, true, false);
    }
}