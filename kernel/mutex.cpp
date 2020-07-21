#include "aex/mutex.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

using Thread = AEX::Proc::Thread;

namespace AEX {
    void Mutex::acquire() {
        volatile size_t count = 0;
        Thread::getCurrent()->addBusy();

        while (!__sync_bool_compare_and_swap(&_lock, false, true)) {
            Thread::getCurrent()->subBusy();

            asm volatile("pause");
            count++;

            if (count > 12212222) {
                int  delta = 0;
                auto name  = Debug::symbol_addr2name((void*) this, &delta);
                if (!name)
                    name = "no idea";

                kpanic("mutex 0x%p <%s+0x%x> hung (val: %i, cpu: %i)\n", this, name, delta, _lock,
                       Sys::CPU::getCurrentID());
            }

            Thread::getCurrent()->addBusy();
        }

        __sync_synchronize();
    }

    void Mutex::release() {
        if (!Thread::getCurrent()->isBusy())
            kpanic("bbb!!!");

        if (!__sync_bool_compare_and_swap(&_lock, true, false))
            kpanic("mutex: Too many releases");

        Thread::getCurrent()->subBusy();

        __sync_synchronize();
    }

    bool Mutex::tryAcquire() {
        Thread::getCurrent()->addBusy();

        bool ret = __sync_bool_compare_and_swap(&_lock, false, true);
        if (!ret)
            Thread::getCurrent()->subBusy();

        __sync_synchronize();

        return ret;
    }
}