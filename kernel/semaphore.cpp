#include "aex/semaphore.hpp"

#include "aex.hpp"

namespace AEX {
    Semaphore::Semaphore(int count, int max) : m_starting(count), m_count(count), m_max(max) {
        m_holder = -max + 1;
    }

    Semaphore::~Semaphore() {
        ASSERT(m_count == m_starting);
    }

    void Semaphore::acquire() {
        while (Mem::atomic_add_fetch(&m_count, 1) > m_max) {
            Mem::atomic_sub_fetch(&m_count, 1);
            Proc::Thread::wait(&m_holder);
        }

        Proc::Thread::current()->held_mutexes++;
        __sync_synchronize();

        Mem::atomic_add_fetch(&m_holder, 1);
    }

    void Semaphore::release() {
        Mem::atomic_sub_fetch(&m_count, 1);
        Mem::atomic_sub_fetch(&m_holder, 1);

        __sync_synchronize();
        Proc::Thread::current()->held_mutexes--;
    }

    bool Semaphore::tryAcquire() {
        if (Mem::atomic_add_fetch(&m_count, 1) > m_max) {
            Mem::atomic_sub_fetch(&m_count, 1);
            return false;
        }

        Proc::Thread::current()->held_mutexes++;
        __sync_synchronize();

        Mem::atomic_add_fetch(&m_holder, 1);

        return true;
    }
}