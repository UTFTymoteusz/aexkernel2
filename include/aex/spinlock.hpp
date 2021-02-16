#pragma once

#include "aex/utility.hpp"

#include <stddef.h>

namespace AEX {
    class ScopeSpinlock;

    class API Spinlock {
        public:
        void acquire();
        void release();

        bool tryAcquire();
        bool isAcquired();

        void acquireRaw();
        void releaseRaw();

        bool tryAcquireRaw();
        bool tryReleaseRaw();

        ScopeSpinlock scope();

        private:
        volatile int m_lock = 0;
    };

    class API ScopeSpinlock {
        public:
        ScopeSpinlock(Spinlock& lock) {
            lock.acquire();

            m_lock = &lock;
        }

        ~ScopeSpinlock() {
            m_lock->release();
        }

        private:
        Spinlock* m_lock;
    };
}