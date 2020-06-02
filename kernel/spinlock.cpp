#include "aex/spinlock.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"

namespace AEX {
    void Spinlock::acquire() {
        volatile size_t count = 0;
        Proc::Thread::getCurrentThread()->addCritical();

        while (!__sync_bool_compare_and_swap(&_lock, false, true)) {
            Proc::Thread::getCurrentThread()->subCritical();

            asm volatile("pause");
            count++;

            if (count > 4212222) {
                kpanic("spinlock 0x%p hung (val: %i, cpu: %i)\n", this, _lock,
                       Sys::CPU::getCurrentCPUID());
            }

            Proc::Thread::getCurrentThread()->addCritical();
        }

        __sync_synchronize();
    }

    void Spinlock::release() {
        if (!Proc::Thread::getCurrentThread()->isCritical())
            kpanic("aaa!!!");

        if (!__sync_bool_compare_and_swap(&_lock, true, false))
            kpanic("spinlock: Too many releases");

        Proc::Thread::getCurrentThread()->subCritical();

        __sync_synchronize();
    }

    bool Spinlock::tryAcquire() {
        Proc::Thread::getCurrentThread()->addCritical();

        bool ret = __sync_bool_compare_and_swap(&_lock, false, true);

        if (!ret)
            Proc::Thread::getCurrentThread()->subCritical();

        return ret;
    }


    void Spinlock::acquireRaw() {
        while (!__sync_bool_compare_and_swap(&_lock, false, true))
            asm volatile("pause");

        __sync_synchronize();
    }

    void Spinlock::releaseRaw() {
        if (!__sync_bool_compare_and_swap(&_lock, true, false))
            kpanic("spinlock: Too many releases");

        __sync_synchronize();
    }

    bool Spinlock::tryAcquireRaw() {
        return __sync_bool_compare_and_swap(&_lock, false, true);
    }
}