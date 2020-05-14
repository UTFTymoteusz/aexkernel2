#pragma once

#include "aex/mem/atomic.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/proc/resource_usage.hpp"
#include "aex/spinlock.hpp"

#include "proc/context.hpp"

#include <stddef.h>

namespace AEX::Proc {
    typedef int tid_t;

    class Process;

    class Thread {
      public:
        enum status_t : uint8_t {
            FRESH    = 0,
            RUNNABLE = 1,
            SLEEPING = 2,
            BLOCKED  = 3,
        };

        tid_t tid;

        Context context;

        Spinlock          lock;
        Mem::ref_counter* refs = new Mem::ref_counter(1);

        status_t status;
        union {
            uint64_t wakeup_at;
        };

        Process* parent;

        Thread() = default;
        Thread(Process* parent);
        Thread(Process* parent, void* entry, void* stack, size_t stack_size, VMem::Pagemap* pagemap,
               bool usermode = false, bool dont_add = false);

        ~Thread();

        /**
         * Gets the parent process of the thread.
         * @return The SmartPointer to the parent process.
         */
        Mem::SmartPointer<Process> getProcess();

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
         * Gets the currently running thread's ID.
         * @return Thread ID of the currently running thread.
         */
        static tid_t getCurrentTID();

        /**
         * Adds the thread to the run queue and sets its status as RUNNABLE.
         */
        void start();

        Mem::SmartPointer<Thread> getSmartPointer();

        void setStatus(status_t status);

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
            return Mem::atomic_read(&_busy) > 0 || Mem::atomic_read(&_critical) > 0;
        }

        /**
         * Adds 1 to the thread's critical counter. If _critical is greater than 0, the thread
         * cannot be interrupted or killed.
         */
        inline void addCritical() {
            Mem::atomic_add(&_critical, (uint16_t) 1);
        }

        /**
         * Subtracts 1 from the thread's critical counter. If _critical is greater than 0, the
         * thread cannot be interrupted or killed.
         */
        inline void subCritical() {
            Mem::atomic_sub(&_critical, (uint16_t) 1);
        }

        /**
         * Checks the thread's critical counter. If _critical is greater than 0, the thread cannot
         * be interrupted or killed.
         * @return Whenever the thread is "critical".
         */
        inline bool isCritical() {
            return Mem::atomic_read(&_critical) > 0;
        }

      private:
        uint16_t _busy;
        uint16_t _critical;
    };
}