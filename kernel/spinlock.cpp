#include "aex/spinlock.hpp"

#include "aex/kpanic.hpp"
#include "aex/printk.hpp"

#include "sys/cpu.hpp"

namespace AEX {
    void Spinlock::acquire() {
        while (!__sync_bool_compare_and_swap(&_lock, false, true))
            asm volatile("pause");

        __sync_synchronize();
    }

    void Spinlock::release() {
        if (!__sync_bool_compare_and_swap(&_lock, true, false))
            kpanic("spinlock: Too many releases");

        __sync_synchronize();
    }

    bool Spinlock::tryAcquire() {
        return __sync_bool_compare_and_swap(&_lock, false, true);
    }
}