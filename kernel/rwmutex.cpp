#include "aex/rwmutex.hpp"

#include "aex/assert.hpp"
#include "aex/proc/thread.hpp"

using namespace AEX::Proc;

namespace AEX {
    void RWMutex::acquire_read() {
        m_lock.acquire();

        while (Mem::atomic_read(&m_write)) {
            m_lock.release();
            m_lock.acquire();
        }

        Thread::current()->addBusy();
        Mem::atomic_add(&m_read, 1);

        m_lock.release();
        __sync_synchronize();
    }

    void RWMutex::acquire_write() {
        m_lock.acquire();

        while (Mem::atomic_read(&m_write)) {
            m_lock.release();
            m_lock.acquire();
        }

        Thread::current()->addBusy();
        Mem::atomic_add(&m_write, 1);

        m_lock.release();
        __sync_synchronize();
    }

    bool RWMutex::tryAcquire_read() {
        m_lock.acquire();

        if (Mem::atomic_read(&m_write)) {
            m_lock.release();
            return false;
        }

        Thread::current()->addBusy();
        Mem::atomic_add(&m_read, 1);

        m_lock.release();
        __sync_synchronize();

        return true;
    }

    bool RWMutex::tryAcquire_write() {
        m_lock.acquire();

        if (Mem::atomic_read(&m_write)) {
            m_lock.release();
            return false;
        }

        Thread::current()->addBusy();
        Mem::atomic_add(&m_write, 1);

        m_lock.release();
        __sync_synchronize();

        return true;
    }

    void RWMutex::upgrade() {
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

    void RWMutex::downgrade() {
        m_lock.acquire();

        Mem::atomic_add(&m_read, 1);
        Mem::atomic_sub(&m_write, 1);

        m_lock.release();
        __sync_synchronize();
    }

    void RWMutex::release_read() {
        AEX_ASSERT(Mem::atomic_read(&m_read) > 0);

        Mem::atomic_sub(&m_read, 1);
        Thread::current()->subBusy();

        __sync_synchronize();
    }

    void RWMutex::release_write() {
        AEX_ASSERT(Mem::atomic_read(&m_write) > 0);

        Mem::atomic_sub(&m_write, 1);
        Thread::current()->subBusy();

        __sync_synchronize();
    }

    ScopeRWMutexRead RWMutex::scope_read() {
        return ScopeRWMutexRead(*this);
    }

    ScopeRWMutexWrite RWMutex::scope_write() {
        return ScopeRWMutexWrite(*this);
    }

    bool RWMutex::acquired_read() {
        auto scope = m_lock.scope();
        return Mem::atomic_read(&m_read);
    }

    bool RWMutex::acquired_write() {
        auto scope = m_lock.scope();
        return Mem::atomic_read(&m_write);
    }
}