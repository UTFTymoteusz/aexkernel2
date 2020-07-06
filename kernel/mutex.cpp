#include "aex/mutex.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"

namespace AEX {
    void Mutex::acquire() {
        volatile size_t count = 0;
        Proc::Thread::getCurrentThread()->addBusy();

        while (!__sync_bool_compare_and_swap(&_lock, false, true)) {
            Proc::Thread::getCurrentThread()->subBusy();

            asm volatile("pause");
            count++;

            if (count > 12212222) {
                int  delta = 0;
                auto name  = Debug::symbol_addr2name((void*) this, &delta);
                if (!name)
                    name = "no idea";

                kpanic("mutex 0x%p <%s+0x%x> hung (val: %i, cpu: %i)\n", this, name, delta, _lock,
                       Sys::CPU::getCurrentCPUID());
            }

            Proc::Thread::getCurrentThread()->addBusy();
        }

        __sync_synchronize();
    }

    void Mutex::release() {
        if (!Proc::Thread::getCurrentThread()->isBusy())
            kpanic("bbb!!!");

        if (!__sync_bool_compare_and_swap(&_lock, true, false))
            kpanic("mutex: Too many releases");

        Proc::Thread::getCurrentThread()->subBusy();

        __sync_synchronize();
    }

    bool Mutex::tryAcquire() {
        Proc::Thread::getCurrentThread()->addBusy();

        bool ret = __sync_bool_compare_and_swap(&_lock, false, true);
        if (!ret)
            Proc::Thread::getCurrentThread()->subBusy();

        return ret;
    }
}