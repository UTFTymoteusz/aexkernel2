#pragma once

#include "aex/arch/proc/context.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/mem/paging.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/proc/resource_usage.hpp"
#include "aex/spinlock.hpp"

namespace AEX::IPC {
    class Event;
}

namespace AEX::Proc {
    typedef int tid_t;

    class Process;
    class Event;
    class Context;

    enum thread_status_t : uint8_t {
        THREAD_FRESH    = 0,
        THREAD_RUNNABLE = 1,
        THREAD_SLEEPING = 2,
        THREAD_BLOCKED  = 3,
        THREAD_DEAD     = 4,
    };

    class Thread {
        public:
        static constexpr auto USER_STACK_SIZE   = 16384;
        static constexpr auto KERNEL_STACK_SIZE = 16384;
        static constexpr auto FAULT_STACK_SIZE  = 16384;

        struct state {
            uint16_t busy;
            uint16_t critical;

            thread_status_t status;
        };

        tid_t tid;

        Context* context;

        Spinlock          lock;
        Mem::ref_counter* refs = new Mem::ref_counter(1);

        thread_status_t status;
        union {
            uint64_t wakeup_at;
        };

        Process* parent;

        size_t user_stack;
        size_t kernel_stack;
        size_t fault_stack;

        int user_stack_size;
        int kernel_stack_size;
        int fault_stack_size;

        Thread();
        Thread(Process* parent);
        Thread(Process* parent, void* entry, size_t stack_size, Mem::Pagemap* pagemap,
               bool usermode = false, bool dont_add = false);

        ~Thread();

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
         * @returns Pointer to the currently running thread.
         */
        static Thread* getCurrent();

        /**
         * Gets the currently running thread's ID.
         * @returns Thread ID of the currently running thread.
         */
        static tid_t getCurrentTID();

        /**
         * Checks if the currently running thread has received the abort signal.
         * @returns True if abort has been received.
         */
        static bool shouldExit();

        [[noreturn]] static void exit();

        /**
         * Gets the parent process of the thread.
         * @returns The SmartPointer to the parent process.
         */
        Mem::SmartPointer<Process> getProcess();

        /**
         * Adds the thread to the run queue and sets its status as RUNNABLE.
         */
        void start();

        /**
         * Waits until the thread finishes execution.
         */
        void join();

        /**
         * Sets the arguments of the thread.
         * @param args Arguments.
         */
        template <typename... U>
        void setArguments(U... args) {
            context->setArguments(args...);
        }

        /**
         * Aborts the thread.
         * @param block Whenever to wait for the thread to exit fully.
         */
        void abort(bool block = false);

        /**
         * Checks if the thread has received the abort signal.
         * @returns True if abort has been received.
         */
        bool isAbortSet();

        bool isFinished();

        /**
         * Returns a smart pointer to the thread.
         * @returns The smart pointer.
         */
        Mem::SmartPointer<Thread> getSmartPointer();

        /**
         * Sets the status of the thread.
         * @param status Status to set to.
         */
        void setStatus(thread_status_t status);

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
            uint16_t busy = Mem::atomic_sub_fetch(&_busy, (uint16_t) 1);

            if (Mem::atomic_read(&_abort) == 1 && !isFinished() && busy == 0)
                Thread::exit();
        }

        /**
         * Checks the thread's busy counter. If _busy is greater than 0, the thread cannot
         * be killed.
         * @returns Whenever the thread is "busy".
         */
        inline bool isBusy() {
            return Mem::atomic_read(&_busy) > 0;
        }

        inline uint16_t getBusy() {
            return Mem::atomic_read(&_busy);
        }

        inline void setBusy(uint16_t busy) {
            _busy = busy;
        }

        /**
         * Adds 1 to the thread's critical counter. If _critical is greater than 0, the thread
         * cannot be interrupted or killed.
         */
        void addCritical();

        /**
         * Subtracts 1 from the thread's critical counter. If _critical is greater than 0, the
         * thread cannot be interrupted or killed.
         */
        void subCritical();

        /**
         * Checks the thread's critical counter. If _critical is greater than 0, the thread cannot
         * be interrupted or killed.
         * @returns Whenever the thread is "critical".
         */
        inline bool isCritical() {
            return Mem::atomic_read(&_critical) > 0;
        }

        inline uint16_t getCritical() {
            return Mem::atomic_read(&_critical);
        }

        inline void setCritical(uint16_t critical) {
            _critical = critical;
        }

        state saveState();

        void loadState(state& _state);

        void announceExit();

        private:
        IPC::Event* _exit_event = nullptr;

        public:
        uint16_t _busy     = 0;
        uint16_t _critical = 0;

        uint8_t _abort    = 0;
        uint8_t _finished = 0;
    };

    typedef Mem::SmartPointer<Thread> Thread_SP;
}