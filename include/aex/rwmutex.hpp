#pragma once

#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

namespace AEX {
    class ScopeRWMutexRead;
    class ScopeRWMutexWrite;

    class API RWMutex {
        public:
        void acquire_read();
        void acquire_write();

        bool tryAcquire_read();
        bool tryAcquire_write();

        ScopeRWMutexRead  scope_read();
        ScopeRWMutexWrite scope_write();

        // Upgrades a reader to a writer
        void upgrade();
        // Downgrades a writer to a reader
        void downgrade();

        void release_read();
        void release_write();

        bool acquired_read();
        bool acquired_write();

        private:
        int m_read  = 0;
        int m_write = 0;

        Spinlock m_lock;
    };

    class API ScopeRWMutexRead {
        public:
        ScopeRWMutexRead(RWMutex& lock) {
            lock.acquire_read();

            m_lock = &lock;
        }

        ~ScopeRWMutexRead() {
            m_lock->release_read();
        }

        private:
        RWMutex* m_lock;
    };

    class API ScopeRWMutexWrite {
        public:
        ScopeRWMutexWrite(RWMutex& lock) {
            lock.acquire_write();

            m_lock = &lock;
        }

        ~ScopeRWMutexWrite() {
            m_lock->release_write();
        }

        private:
        RWMutex* m_lock;
    };
}