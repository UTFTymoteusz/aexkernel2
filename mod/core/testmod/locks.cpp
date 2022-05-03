#include "aex.hpp"

using namespace AEX;

Semaphore* sem;

void locker_a() {
    sem->acquire();
    printk("testmod: a acquired\n");

    Proc::Thread::sleep(150);

    sem->release();
    printk("testmod: a released\n");
}

void locker_b() {
    sem->acquire();
    printk("testmod: b acquired\n");

    Proc::Thread::sleep(100);

    sem->release();
    printk("testmod: b released\n");
}

void locker_c() {
    sem->acquire();
    printk("testmod: c acquired\n");

    Proc::Thread::sleep(150);

    sem->release();
    printk("testmod: c released\n");
}

void locker_d() {
    sem->acquire();
    printk("testmod: d acquired\n");

    Proc::Thread::sleep(50);

    sem->release();
    printk("testmod: d released\n");
}

void test_locks() {
    sem = new Semaphore(0, 2);

    auto thread_a = Proc::threaded_call(locker_a);
    auto thread_b = Proc::threaded_call(locker_b);
    auto thread_c = Proc::threaded_call(locker_c);
    auto thread_d = Proc::threaded_call(locker_d);

    thread_a->join();
    thread_b->join();
    thread_c->join();
    thread_d->join();

    delete sem;
}