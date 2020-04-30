#pragma once

namespace AEX {
    class Spinlock {
      public:
        void acquire();
        void release();

        bool tryAcquire();

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