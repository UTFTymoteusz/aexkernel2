#include "aex/sys/irq.hpp"

#include "aex/arch/sys/cpu.hpp"
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
        void addFunc(void (*func)(void*), void* arg) {
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

        void callAll() {
            for (size_t i = 0; i < count; i++)
                if (funcs[i])
                    funcs[i](args[i]);
        }
    };

    size_t  queue_waiting = 0;
    uint8_t queue[8192];

    handler_array immediate_handlers[32] = {};
    handler_array threaded_handlers[32]  = {};

    Mem::SmartPointer<Thread> handler_thread;

    Spinlock append_lock;
    Spinlock handler_lock;

    void irq_handler();

    void init_proc() {
        auto thread = new Thread(nullptr, (void*) irq_handler, 4096, nullptr);

        handler_thread = thread->getSmartPointer();
        handler_thread->start();
    }

    void handle_irq(uint8_t irq) {
        static size_t append_pos = 0;

        if (irq >= 32)
            kpanic("irq >= 32 wtf");

        Thread::getCurrent()->addCritical();
        immediate_handlers[irq].callAll();

        append_lock.acquireRaw();

        queue[append_pos] = irq;

        append_pos++;
        if (append_pos >= sizeof(queue))
            append_pos = 0;

        Mem::atomic_add(&queue_waiting, (size_t) 1);

        handler_thread->setStatus(Proc::THREAD_RUNNABLE);

        append_lock.releaseRaw();
        Thread::getCurrent()->subCritical();

        // enqueue for the threaded one
        // register_threaded_handler
    }

    void register_handler(uint8_t irq, void (*func)(void* arg), void* arg) {
        immediate_handlers[irq].addFunc(func, arg);
    }

    void register_threaded_handler(uint8_t irq, void (*func)(void* arg), void* arg) {
        threaded_handlers[irq].addFunc(func, arg);
    }

    void irq_handler() {
        static size_t read_pos = 0;

        while (true) {
            handler_lock.acquire();

            if (Mem::atomic_read(&queue_waiting) == 0) {
                Sys::CPU::nointerrupts();

                handler_thread->setStatus(Proc::THREAD_BLOCKED);

                handler_lock.release();
                Sys::CPU::interrupts();

                Thread::yield();
                continue;
            }

            handler_lock.release();

            while (Mem::atomic_read(&queue_waiting) > 0) {
                Mem::atomic_sub(&queue_waiting, (size_t) 1);

                uint8_t irq = queue[read_pos];

                read_pos++;
                if (read_pos >= sizeof(queue))
                    read_pos = 0;

                threaded_handlers[irq].callAll();
            }
        }
    }
}