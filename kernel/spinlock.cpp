#include "aex/spinlock.hpp"

#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"

#include "sys/cpu.hpp"

namespace AEX {
    void Spinlock::acquire() {
        Proc::Thread::getCurrentThread()->addCritical();

        while (!__sync_bool_compare_and_swap(&_lock, false, true))
            asm volatile("pause");

        __sync_synchronize();
    }

    void Spinlock::release() {
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