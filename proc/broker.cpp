#include "aex/proc/broker.hpp"

#include "aex/ipc/messagequeue.hpp"
#include "aex/proc/thread.hpp"

#include "proc/broker.hpp"

namespace AEX::Proc {
    struct broker_request {
        void* (*func)(void* arg);
        void* arg;
    };

    IPC::MessageQueue broker_queue;
    Thread*           broker_thread;

    void broker_loop();

    void broker_init() {
        broker_thread =
            Thread::create(1, (void*) broker_loop, Thread::KERNEL_STACK_SIZE, nullptr).value;
        broker_thread->start();
        broker_thread->detach();

        new (&broker_queue) IPC::MessageQueue();
    }

    void broker(void* (*func)(void* arg), void* arg) {
        auto request = broker_request();

        request.func = func;
        request.arg  = arg;

        broker_queue.writeObject(request);
    }

    void broker_loop() {
        while (true) {
            auto request = broker_queue.readObject<broker_request>();
            request.func(request.arg);
        }
    }
}
