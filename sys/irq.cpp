#include "aex/sys/irq.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/spinlock.hpp"

#include "sys/irq.hpp"
#include "sys/irq_i.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX::Mem;

using Thread = AEX::Proc::Thread;

namespace AEX::Sys::IRQ {
    struct handler_array {
        size_t count          = 0;
        void (**funcs)(void*) = nullptr;
        void** args           = nullptr;

        // need some locks here
        void add(void (*func)(void*), void* arg) {
            if (!funcs) {
                count = 1;

                funcs    = (decltype(funcs)) Heap::malloc(sizeof(func) * count);
                funcs[0] = func;

                args    = (decltype(args)) Heap::malloc(sizeof(arg) * count);
                args[0] = arg;

                return;
            }

            for (size_t i = 0; i < count; i++)
                if (funcs[i] == func && args[i] == arg)
                    return;

            count++;
            funcs            = (decltype(funcs)) Heap::realloc(funcs, sizeof(func) * count);
            funcs[count - 1] = func;

            args            = (decltype(args)) Heap::realloc(args, sizeof(arg) * count);
            args[count - 1] = arg;
        }

        void call() {
            for (size_t i = 0; i < count; i++)
                if (funcs[i])
                    funcs[i](args[i]);
        }
    };

    handler_array handlers[32] = {};

    Spinlock append_lock;
    Spinlock handler_lock;

    void handle(uint8_t irq) {
        AEX_ASSERT(irq < 32);

        // We need to steal the state of thread so nothing messes with us
        // State saving is possible because only the executing thread changes it's criticality and
        // busies.

        auto thread = Thread::getCurrent();
        auto state  = thread->saveState();

        thread->setCritical(1);
        thread->setBusy(0);
        thread->setStatus(Proc::TS_RUNNABLE);

        handlers[irq].call();

        thread->loadState(state);
    }

    void register_handler(uint8_t irq, void (*func)(void* arg), void* arg) {
        handlers[irq].add(func, arg);
    }
}