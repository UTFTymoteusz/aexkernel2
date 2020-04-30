#pragma once

#include "aex/mem/vmem.hpp"
#include "aex/rcparray.hpp"
#include "aex/spinlock.hpp"

#include "proc/context.hpp"
#include "proc/resource_usage.hpp"

#include <stddef.h>

namespace AEX::Proc {
    typedef int tid_t;

    class Process;

    class Thread {
      public:
        enum state : uint8_t {
            FRESH    = 0,
            RUNNABLE = 1,
            SLEEPING = 2,
            BLOCKED  = 3,
        };

        Context  context;
        Spinlock lock;

        state status;
        union {
            size_t wakeup_at;
        };

        Process* parent;

        Thread() = default;
        Thread(Process* parent);
        Thread(Process* parent, void* entry, void* stack, size_t stack_size,
               VMem::Pagemap* pagemap);

        /**
         * Adds the thread to the run queue and sets its status as RUNNABLE.
         */
        void start();

        /**
         * Gets the parent process of the thread.
         * @return The RCPArray::Pointer to the parent process.
         */
        RCPArray<Process>::Pointer getProcess();

        static void yield();

        static void sleep(int ms);

        /**
         * Gets the currently running thread.
         * @return Pointer to the currently running thread.
         */
        static Thread* getCurrentThread();

      private:
    };
}