#pragma once

namespace AEX {
    class ScopeMutex;

    class Mutex {
        public:
        void acquire();
        void release();

        bool tryAcquire();

        ScopeMutex scope();

        private:
        volatile int m_lock = 0;
    };

    class ScopeMutex {
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
}