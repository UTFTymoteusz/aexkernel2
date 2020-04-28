#pragma once

namespace AEX {
    class Spinlock {
      public:
        void acquire();
        void release();

      private:
        volatile int lock = 0;
    };
}