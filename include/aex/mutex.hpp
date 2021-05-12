#pragma once

#include "aex/utility.hpp"

namespace AEX {
    class ScopeMutex;

    class API Mutex {
        public:
        void acquire();
        void release();

        bool tryAcquire();

        ScopeMutex scope();

        private:
        volatile int m_lock   = 0;
        void*        m_thread = 0;
    };

    class API ScopeMutex {
        public:
        ScopeMutex(Mutex& lock) {
            lock.acquire();

            m_lock = &lock;
        }

        ~ScopeMutex() {
            m_lock->release();
        }

        private:
        Mutex* m_lock;
    };

    class API ReleaseScopeMutex {
        public:
        ReleaseScopeMutex() {
            m_lock = nullptr;
        }

        ReleaseScopeMutex(Mutex& lock) {
            m_lock = &lock;
        }

        ~ReleaseScopeMutex() {
            if (m_lock)
                m_lock->release();
        }

        private:
        Mutex* m_lock;
    };
}