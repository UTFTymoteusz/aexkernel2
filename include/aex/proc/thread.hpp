#pragma once

#include "aex/mem/atomic.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/proc/resource_usage.hpp"
#include "aex/rcparray.hpp"
#include "aex/spinlock.hpp"

#include "proc/context.hpp"

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
            uint64_t wakeup_at;
        };

        Process* parent;

        Thread() = default;
        Thread(Process* parent);
        Thread(Process* parent, void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap,
               bool usermode = false, bool dont_add = false);

        /**
         * Gets the parent process of the thread.
         * @return The RCPArray::Pointer to the parent process.
         */
        RCPArray<Process>::Pointer getProcess();

        /**
         * Yields the currently executing thread's CPU timeshare. Will return immediately if no
         * other threads are available.
         */
        static void yield();

        /**
         * Sleeps the currently running thread.
         * @param ms Amount of time to sleep for in milliseconds.
         */
        static void sleep(int ms);

        /**
         * Gets the currently running thread.
         * @return Pointer to the currently running thread.
         */
        static Thread* getCurrentThread();

        /**
         * Adds the thread to the run queue and sets its status as RUNNABLE.
         */
        void start();

        // pls atomic<T> later
        /**
         * Adds 1 to the thread's busy counter. If _busy is greater than 0, the thread cannot be
         * killed.
         */
        inline void addBusy() {
            Mem::atomic_add(&_busy, (uint16_t) 1);
        }

        /**
         * Subtracts 1 from the thread's busy counter. If _busy is greater than 0, the thread cannot
         * be killed.
         */
        inline void subBusy() {
            Mem::atomic_sub(&_busy, (uint16_t) 1);
        }

        /**
         * Checks the thread's busy counter. If _busy is greater than 0, the thread cannot
         * be killed.
         * @return Whenever the thread is "busy".
         */
        inline bool isBusy() {
            return Mem::atomic_read(&_busy) > 0;
        }

      private:
        uint16_t _busy;
    };
}