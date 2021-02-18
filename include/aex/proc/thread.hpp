#pragma once

#include "aex/arch/proc/context.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/proc/types.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

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

    class API Thread {
        public:
        static constexpr auto USER_STACK_SIZE   = 16384;
        static constexpr auto KERNEL_STACK_SIZE = 16384;
        static constexpr auto FAULT_STACK_SIZE  = 32768;
        static constexpr auto AUX_STACK_SIZE    = 4096;

        struct state {
            uint16_t busy;
            uint16_t critical;

            thread_status_t status;
        };

        // Don't change the order of these or the kernel will go boom boom
        Thread* self = this; // 0x00
        tid_t   tid;         // 0x08

        Context* context;     // 0x10
        Context* context_aux; // 0x18

        error_t errno; // 0x20

        size_t saved_stack; // 0x28

        // Safe to change again
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
        size_t aux_stack;

        int user_stack_size;
        int kernel_stack_size;
        int fault_stack_size;
        int aux_stack_size;

        bool faulting;
        bool in_signal;

        void* tls;

        int sched_counter;

        Thread();
        Thread(Process* parent);

        ~Thread();

        [[nodiscard]] static optional<Thread*> create(pid_t parent, void* entry, size_t stack_size,
                                                      Mem::Pagemap* pagemap, bool usermode = false,
                                                      bool dont_add = false);

        static void yield();
        static void sleep(uint64_t ms);
        static void usleep(uint64_t ns);
        static void exit();

        static Thread* current();

        error_t start();
        error_t join();
        error_t detach();
        error_t abort(bool force = false);
        bool    aborting();
        bool    interrupted();

        void _abort(bool force = false);
        void finish();

        Process* getProcess();

        // Make this actually do what it says it'll do
        /**
         * Makes the CPU that's executing the thread back off and acquires the lock.
         **/
        void take_lock();

        // IPC Stuff
        /**
         *
         **/
        error_t signal(IPC::siginfo_t& info);
        error_t sigret();

        /**
         * Sets the arguments of the thread.
         * @param args Arguments.
         **/
        template <typename... U>
        void setArguments(U... args) {
            context->setArguments(args...);
        }

        /**
         * Sets the status of the thread.
         * @param status Status to set to.
         **/
        void setStatus(thread_status_t status);

        // pls atomic<T> later
        /**
         * Adds 1 to the thread's busy counter. If m_busy is greater than 0, the thread cannot be
         * killed.
         **/
        inline void addBusy() {
            Mem::atomic_add(&m_busy, (uint16_t) 1);
        }

        /**
         * Subtracts 1 from the thread's busy counter. If m_busy is greater than 0, the thread
         * cannot be killed.
         **/
        void subBusy();

        /**
         * Checks the thread's busy counter. If m_busy is greater than 0, the thread cannot
         * be killed.
         * @returns Whenever the thread is "busy".
         **/
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
         **/
        void addCritical();

        /**
         * Subtracts 1 from the thread's critical counter. If m_critical is greater than 0, the
         * thread cannot be interrupted or killed.
         **/
        void subCritical();

        /**
         * Checks the thread's critical counter. If m_critical is greater than 0, the thread cannot
         * be interrupted or killed.
         * @returns Whenever the thread is "critical".
         **/
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

        Mem::Vector<IPC::siginfo_t> m_pending_signals;

        error_t              _signal(IPC::siginfo_t& info);
        error_t              handleSignal(IPC::siginfo_t& info);
        error_t              enterSignal(IPC::siginfo_t& info);
        [[noreturn]] error_t exitSignal();

        void alloc_stacks(Mem::Pagemap* pagemap, size_t size, bool usermode);
        void alloc_tls(uint16_t size);
        void setup_context(Mem::Pagemap* pagemap, size_t size, void* entry, bool usermode);
    };

    API void add_thread(Thread* thread);
    API void remove_thread(Thread* thread);
}
