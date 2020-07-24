#pragma once

namespace AEX {
    class Spinlock {
        public:
        void acquire();
        void release();

        bool tryAcquire();

        void acquireRaw();
        void releaseRaw();

        bool tryAcquireRaw();
        bool tryReleaseRaw();

        private:
        volatile int _lock = 0;
    };

    class ScopeSpinlock {
        public:
        ScopeSpinlock(Spinlock& lock) {
            lock.acquire();

            _lock = &lock;
        }

        ~ScopeSpinlock() {
            _lock->release();
        }

        private:
        Spinlock* _lock;
    };
}