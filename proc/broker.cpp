#include "aex/proc/broker.hpp"

#include "aex/ipc/messagequeue.hpp"
#include "aex/kpanic.hpp"
#include "aex/proc/thread.hpp"

#include "proc/broker.hpp"

namespace AEX::Proc {
    struct broker_request {
        void* (*func)(void* arg);
        void* arg;
    };

    IPC::MessageQueue broker_queue;
    Thread*           broker_thread;
    broker_request    broker_current;

    void broker_loop();

    void broker_init() {
        broker_thread =
            Thread::create(1, (void*) broker_loop, Thread::KERNEL_STACK_SIZE, nullptr).value;
        broker_thread->start();
        broker_thread->detach();

        new (&broker_queue) IPC::MessageQueue();

        kpanic_hook.subscribe([]() {
            int  delta;
            auto name = Debug::addr2name((void*) broker_current.func, delta) ?: "unknown";

            printk("broker: Currently executing 0x%p <%s+0x%x>\n", broker_current.func, name,
                   delta);
        });
    }

    void broker(void* (*func)(void* arg), void* arg) {
        broker_queue.writeObject(broker_request{
            .func = func,
            .arg  = arg,
        });
    }

    void broker_loop() {
        while (true) {
            broker_current = broker_queue.readObject<broker_request>();
            broker_current.func(broker_current.arg);
            broker_current = {};
        }
    }
}
