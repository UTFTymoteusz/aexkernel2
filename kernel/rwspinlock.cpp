#include "aex/rwspinlock.hpp"

#include "aex/proc/thread.hpp"

using namespace AEX::Proc;

namespace AEX {
    void RWSpinlock::acquire_read() {
        _lock.acquire();

        while (Mem::atomic_read(&_write)) {
            _lock.release();
            _lock.acquire();
        }

        Thread::getCurrent()->addCritical();
        Mem::atomic_add(&_read, 1);

        _lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::acquire_write() {
        _lock.acquire();

        while (Mem::atomic_read(&_write)) {
            _lock.release();
            _lock.acquire();
        }

        Thread::getCurrent()->addCritical();
        Mem::atomic_add(&_write, 1);

        _lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::upgrade() {
        _lock.acquire();

        while (Mem::atomic_read(&_write)) {
            _lock.release();
            _lock.acquire();
        }

        Mem::atomic_add(&_write, 1);
        Mem::atomic_sub(&_read, 1);

        _lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::downgrade() {
        _lock.acquire();

        Mem::atomic_add(&_read, 1);
        Mem::atomic_sub(&_write, 1);

        _lock.release();
        __sync_synchronize();
    }

    void RWSpinlock::release_read() {
        if (Mem::atomic_read(&_read) <= 0)
            kpanic("_read <= 0");

        Mem::atomic_sub(&_read, 1);
        Thread::getCurrent()->subCritical();

        __sync_synchronize();
    }

    void RWSpinlock::release_write() {
        if (Mem::atomic_read(&_write) <= 0)
            kpanic("_read <= 0");

        Mem::atomic_sub(&_write, 1);
        Thread::getCurrent()->subCritical();

        __sync_synchronize();
    }
}