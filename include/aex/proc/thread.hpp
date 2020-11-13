#pragma once

#include "aex/arch/proc/context.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/mem/paging.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/proc/resource_usage.hpp"
#include "aex/proc/types.hpp"
#include "aex/spinlock.hpp"

// pls consider making thread functions accept and return smartpointers
// make the eventbong use an int or whatever

namespace AEX::Proc {
    class Process;
    class Event;
    class Context;

    enum thread_status_t : uint8_t {
        TS_FRESH    = 0x00,
        TS_RUNNABLE = 0x01,
        TS_SLEEPING = 0x02,
        TS_BLOCKED  = 0x04,
        TS_DEAD     = 0x80,
    };

    class Thread {
        public:
        static constexpr auto USER_STACK_SIZE   = 16384;
        static constexpr auto KERNEL_STACK_SIZE = 16384;
        static constexpr auto FAULT_STACK_SIZE  = 32768;

        struct state {
            uint16_t busy;
            uint16_t critical;

            thread_status_t status;
        };

        Thread* self = this;
        tid_t   tid;

        Context* context;
        Spinlock lock;

        thread_status_t status;
        union {
            int64_t wakeup_at;
        };

        Process* parent;
        Thread*  next;
        Thread*  prev;

        void* original_entry;

        size_t user_stack;
        size_t kernel_stack;
        size_t fault_stack;

        int user_stack_size;
        int kernel_stack_size;
        int fault_stack_size;

        bool faulting;

        Thread();
        Thread(Process* parent);

        ~Thread();

        [[nodiscard]] static optional<Thread*> create(pid_t parent, void* entry, size_t stack_size,
                                                      Mem::Pagemap* pagemap, bool usermode = false,
                                                      bool dont_add = false);

        static void yield();
        static void sleep(int ms);
        static void exit();

        static Thread* current();

        error_t start();
        error_t join();
        error_t detach();
        error_t abort();
        bool    aborting();

        void abortInternal();
        void finish();

        Process* getProcess();

        /**
         * Sets the arguments of the thread.
         * @param args Arguments.
         */
        template <typename... U>
        void setArguments(U... args) {
            context->setArguments(args...);
        }

        /**
         * Sets the status of the thread.
         * @param status Status to set to.
         */
        void setStatus(thread_status_t status);

        // pls atomic<T> later
        /**
         * Adds 1 to the thread's busy counter. If m_busy is greater than 0, the thread cannot be
         * killed.
         */
        inline void addBusy() {
            Mem::atomic_add(&m_busy, (uint16_t) 1);
        }

        /**
         * Subtracts 1 from the thread's busy counter. If m_busy is greater than 0, the thread
         * cannot be killed.
         */
        inline void subBusy() {
            uint16_t busy = Mem::atomic_sub_fetch(&m_busy, (uint16_t) 1);

            if (!busy && aborting() && this == Thread::current())
                Thread::exit();
        }

        /**
         * Checks the thread's busy counter. If m_busy is greater than 0, the thread cannot
         * be killed.
         * @returns Whenever the thread is "busy".
         */
        inline bool isBusy() {
            return Mem::atomic_read(&m_busy) > 0;
        }

        inline uint16_t getBusy() {
            return Mem::atomic_read(&m_busy);
        }

        inline void setBusy(uint16_t busy) {
            m_busy = busy;
        }

        /**
         * Adds 1 to the thread's critical counter. If m_critical is greater than 0, the thread
         * cannot be interrupted or killed.
         */
        void addCritical();

        /**
         * Subtracts 1 from the thread's critical counter. If m_critical is greater than 0, the
         * thread cannot be interrupted or killed.
         */
        void subCritical();

        /**
         * Checks the thread's critical counter. If m_critical is greater than 0, the thread cannot
         * be interrupted or killed.
         * @returns Whenever the thread is "critical".
         */
        inline bool isCritical() {
            return Mem::atomic_read(&m_critical) > 0;
        }

        inline uint16_t getCritical() {
            return Mem::atomic_read(&m_critical);
        }

        inline void setCritical(uint16_t critical) {
            m_critical = critical;
        }

        Thread* joiner() {
            return m_joiner;
        }

        bool detached() {
            return m_detached;
        }

        state saveState();
        void  loadState(state& m_state);

        private:
        uint16_t m_busy     = 0;
        uint16_t m_critical = 0;

        bool m_detached = false;
        bool m_aborting = false;

        Thread* m_joiner = nullptr;
    };
}