#include "aex/rwspinlock.hpp"

#include "aex/assert.hpp"
#include "aex/proc/thread.hpp"

using namespace AEX::Proc;

namespace AEX {
    void RWSpinlock::acquire_read() {
        m_lock.acquire();

        while (Mem::atomic_read(&m_write)) {
            m_lock.release();
            m_lock.acquire();
        }

        Thread::current()->addCritical();
        Mem::atomic_add(&m_read, 1);

        m_lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::acquire_write() {
        m_lock.acquire();

        while (Mem::atomic_read(&m_write)) {
            m_lock.release();
            m_lock.acquire();
        }

        Thread::current()->addCritical();
        Mem::atomic_add(&m_write, 1);

        m_lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::upgrade() {
        m_lock.acquire();

        while (Mem::atomic_read(&m_write)) {
            m_lock.release();
            m_lock.acquire();
        }

        Mem::atomic_add(&m_write, 1);
        Mem::atomic_sub(&m_read, 1);

        m_lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::downgrade() {
        m_lock.acquire();

        Mem::atomic_add(&m_read, 1);
        Mem::atomic_sub(&m_write, 1);

        m_lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::release_read() {
        AEX_ASSERT(Mem::atomic_read(&m_read) > 0);

        Mem::atomic_sub(&m_read, 1);
        Thread::current()->subCritical();

        __sync_synchronize();
    }

    void RWSpinlock::release_write() {
        AEX_ASSERT(Mem::atomic_read(&m_write) > 0);

        Mem::atomic_sub(&m_write, 1);
        Thread::current()->subCritical();

        __sync_synchronize();
    }
}