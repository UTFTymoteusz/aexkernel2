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

        while (!__sync_bool_compare_and_swap(&_lock, false, true)) {
            Thread::getCurrent()->subCritical();

            asm volatile("pause");
            count++;

            if (count > 12212222) {
                int  delta = 0;
                auto name  = Debug::symbol_addr2name((void*) this, &delta);
                if (!name)
                    name = "no idea";

                kpanic("spinlock 0x%p <%s+0x%x> hung (val: %i, cpu: %i)\n", this, name, delta,
                       _lock, Sys::CPU::getCurrentID());
            }

            Thread::getCurrent()->addCritical();
        }

        __sync_synchronize();
    }

    void Spinlock::release() {
        if (!Thread::getCurrent()->isCritical())
            kpanic("aaa!!!");

        if (!__sync_bool_compare_and_swap(&_lock, true, false))
            kpanic("spinlock: Too many releases");

        Thread::getCurrent()->subCritical();

        __sync_synchronize();
    }

    bool Spinlock::tryAcquire() {
        Thread::getCurrent()->addCritical();

        bool ret = __sync_bool_compare_and_swap(&_lock, false, true);
        if (!ret)
            Thread::getCurrent()->subCritical();

        __sync_synchronize();

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