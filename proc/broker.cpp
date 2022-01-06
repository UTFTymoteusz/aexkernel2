#include "aex/proc/broker.hpp"

#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/mem/circularbuffer.hpp"
#include "aex/proc/thread.hpp"

#include "proc/broker.hpp"

namespace AEX::Proc {
    struct broker_request {
        void* (*func)(void* arg);
        void* arg;
    };

    Mem::CircularBuffer<broker_request>* broker_queue;
    Thread*                              broker_thread;
    broker_request                       broker_current;

    void broker_loop();

    void broker_init() {
        broker_thread =
            Thread::create(1, (void*) broker_loop, Thread::KERNEL_STACK_SIZE, nullptr).value;
        broker_thread->start();
        broker_thread->detach();

        broker_queue = new Mem::CircularBuffer<broker_request>(64);

        kpanic_hook.subscribe([]() {
            int  delta = 0;
            auto name  = Debug::addr2name((void*) broker_current.func, delta) ?: "unknown";

            printk("broker: Currently executing %p <%s+0x%x>\n", broker_current.func, name, delta);
        });
    }

    void broker(void* (*func)(void* arg), void* arg) {
        broker_queue->write({
            .func = func,
            .arg  = arg,
        });
    }

    void broker_loop() {
        while (true) {
            broker_current = broker_queue->read();
            broker_current.func(broker_current.arg);
            broker_current = {};
        }
    }
}
