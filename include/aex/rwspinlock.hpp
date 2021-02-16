#pragma once

#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

namespace AEX {
    class API RWSpinlock {
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
        int m_read  = 0;
        int m_write = 0;

        Spinlock m_lock;
    };

    class API ScopeRWSpinlockRead {
        public:
        ScopeRWSpinlockRead(RWSpinlock& lock) {
            lock.acquire_read();

            m_lock = &lock;
        }

        ~ScopeRWSpinlockRead() {
            m_lock->release_read();
        }

        private:
        RWSpinlock* m_lock;
    };

    class API ScopeRWSpinlockWrite {
        public:
        ScopeRWSpinlockWrite(RWSpinlock& lock) {
            lock.acquire_write();

            m_lock = &lock;
        }

        ~ScopeRWSpinlockWrite() {
            m_lock->release_write();
        }

        private:
        RWSpinlock* m_lock;
    };
}