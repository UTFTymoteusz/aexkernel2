#pragma once

#include "aex/mem/vmem.hpp"
#include "aex/spinlock.hpp"

#include "proc/context.hpp"

#include <stddef.h>

namespace AEX::Proc {
    class Thread {
      public:
        enum state : uint8_t {
            RUNNABLE = 0,
            SLEEPING = 1,
            BLOCKED  = 2,
        };

        Context  context;
        Spinlock lock;

        state status;
        union {
            size_t wakeup_at;
        };

        Thread() = default;
        Thread(void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap);

        static void yield();

        static void sleep(int ms);

      private:
    };
}