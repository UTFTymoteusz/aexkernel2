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
        void          trace();

        private:
        static constexpr auto TRACE_DEPTH = 2;

        volatile int m_lock               = 0;
        void*        m_thread             = 0;
        void*        callers[TRACE_DEPTH] = {};

        void acquired(bool raw = false);
        void released(bool raw = false);
        void fail(const char* reason);
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