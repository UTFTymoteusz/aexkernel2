#pragma once

#include "aex/spinlock.hpp"

namespace AEX {
    class RWSpinlock {
        public:
        void acquire_read();
        void acquire_write();

        // Upgrades a reader to a writer
        void upgrade();
        // Downgrades a writer to a reader
        void downgrade();

        void release_read();
        void release_write();

        private:
        int _read  = 0;
        int _write = 0;

        Spinlock _lock;
    };

    class ScopeRWSpinlockRead {
        public:
        ScopeRWSpinlockRead(RWSpinlock& lock) {
            lock.acquire_read();

            _lock = &lock;
        }

        ~ScopeRWSpinlockRead() {
            _lock->release_read();
        }

        private:
        RWSpinlock* _lock;
    };

    class ScopeRWSpinlockWrite {
        public:
        ScopeRWSpinlockWrite(RWSpinlock& lock) {
            lock.acquire_write();

            _lock = &lock;
        }

        ~ScopeRWSpinlockWrite() {
            _lock->release_write();
        }

        private:
        RWSpinlock* _lock;
    };
}