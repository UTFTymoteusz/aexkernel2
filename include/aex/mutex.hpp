#pragma once

namespace AEX {
    class Mutex {
        public:
        void acquire();
        void release();

        bool tryAcquire();

        private:
        volatile int _lock = 0;
    };

    class ScopeMutex {
        public:
        ScopeMutex(Mutex& lock) {
            lock.acquire();

            _lock = &lock;
        }

        ~ScopeMutex() {
            _lock->release();
        }

        private:
        Mutex* _lock;
    };
}