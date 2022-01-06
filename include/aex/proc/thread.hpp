#pragma once

#include "aex/arch/ipc/signal.hpp"
#include "aex/arch/proc/context.hpp"
#include "aex/arch/sys/cpu.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/ipc/sigqueue.hpp"
#include "aex/ipc/types.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/proc/guards.hpp"
#include "aex/proc/types.hpp"
#include "aex/spinlock.hpp"
#include "aex/utility.hpp"

// pls consider making thread functions accept and return smartpointers
// make the eventbong use an int or whatever

namespace AEX::IPC {
    class Event;
}

namespace AEX::Proc {
    class Process;
    class Event;
    class Context;

    class API Thread {
        public:
        static constexpr auto USER_STACK_SIZE   = 16384;
        static constexpr auto KERNEL_STACK_SIZE = 16384;
        static constexpr auto FAULT_STACK_SIZE  = 32768;

        struct state {
            uint16_t signability;
            uint16_t critical;
            status_t status;
        };

        Thread* self = this; // 0x00

        // Do not change the order of these or the kernel will go boom boom
        struct {
            tid_t    tid;          // 0x08
            Context* context;      // 0x10
            void*    unused;       // 0x18
            error_t  errno;        // 0x20
            size_t   sigret_stack; // 0x28
        };

        Spinlock lock;
        Spinlock sigcheck_lock;

        volatile status_t status;
        int64_t           wakeup_at;

        CriticalGuard    criticalGuard    = CriticalGuard(this);
        SignabilityGuard signabilityGuard = SignabilityGuard(this);

        Process*    parent;
        IPC::Event* event;

        void* original_entry;

        stack user_stack;
        stack kernel_stack;
        stack fault_stack;

        bool faulting;
        ALIGN(16) Context fault_recovery;

        void* tls;
        void* retval = nullptr;

        int sched_counter;
        int held_mutexes;

        Thread* next;
        Thread* prev;

        Thread();
        Thread(Process* parent);
        ~Thread();

        static Thread* current();

        [[nodiscard]] static optional<Thread*> create(pid_t parent, void* entry, size_t stack_size,
                                                      Mem::Pagemap* pagemap, bool usermode = false,
                                                      bool dont_add = false);

        static void yield();
        static void sleep(uint64_t ms);
        static void nsleep(uint64_t ns);
        static void exit(void* retval = nullptr, bool ignoreBusy = false);
        static void exit_implicit() {
            exit(nullptr, true);
        }

        error_t         start();
        optional<void*> join();
        error_t         detach();
        error_t         abort(bool force = false);
        bool            aborting();
        bool            interrupted();

        /**
         * Exits the thread if it is supposed to get aborted. Keep in mind that this method can
         * restore interrupts, so use it only where that is safe.
         */
        void abortchk();

        Process* getProcess();

        // IPC Stuff
        error_t                  signal(IPC::siginfo_t& info);
        error_t                  sigret();
        bool                     sigchk(IPC::state_combo scmb = {});
        error_t                  sighandle(IPC::siginfo_t& info, IPC::state_combo scmb);
        void                     sigthasync(bool asyncthrdsig);
        void                     sigwait(bool sigwait);
        optional<IPC::siginfo_t> sigget(IPC::sigset_t* set = nullptr);
        IPC::sigset_t            sigmask();
        void                     sigmask(const IPC::sigset_t& mask);
        IPC::sigset_t            sigpending();

        /**
         * Sets the arguments of the thread.
         * @param args Arguments.
         **/
        template <typename... U>
        void setArguments(U... args) {
            context->setArguments(args...);
        }

        /**
         * Returns true if the thread is currently executing kernel code.
         * @returns Whether the thread is executing kernel code.
         **/
        bool isBusy();

        // pls atomic<T> later
        /**
         * Adds 1 to the thread's signability counter. If m_signability is greater than 0, the
         * thread can be interrupted by signals.
         **/
        void addSignability();

        /**
         * Subtracts 1 from the thread's signability counter. If m_signability is greater than 0,
         * the thread can be interrupted by signals.
         **/
        void subSignability();

        inline bool isSignalable() {
            return Mem::atomic_read(&m_signability) > 0;
        }

        inline uint16_t getSignability() {
            return Mem::atomic_read(&m_signability);
        }

        inline void setSignability(uint16_t signability) {
            m_signability = signability;
        }

        /**
         * Adds 1 to the thread's critical counter. If m_critical is greater than 0, the thread
         * cannot be rescheduled or killed.
         **/
        void addCritical();

        /**
         * Subtracts 1 from the thread's critical counter. If m_critical is greater than 0, the
         * thread cannot be rescheduled or killed.
         **/
        void subCritical();

        /**
         * Checks the thread's critical counter. If m_critical is greater than 0, the thread cannot
         * be rescheduled or killed.
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
        uint32_t m_signability = 0;
        uint32_t m_critical    = 0;

        bool m_detached   = false;
        bool m_aborting   = false;
        bool m_retval_set = false;

        Thread*       m_joiner = nullptr;
        IPC::SigQueue m_sigqueue;

        bool m_asyncthrdsig = false;
        bool m_sigwait      = false;

        error_t              sigenter(IPC::siginfo_t& info, IPC::state_combo scmb);
        [[noreturn]] error_t sigexit();

        error_t sigpush(uint8_t id, IPC::sigaction& action, IPC::siginfo_t& info,
                        IPC::state_combo scmb);
        error_t sigpop();

        void finish();

        error_t _signal(IPC::siginfo_t& info);
        void    _abort(bool force = false);

        void alloc_stacks(Mem::Pagemap* pagemap, size_t size, bool usermode);
        void create_tls();
        void setup_context(Mem::Pagemap* pagemap, size_t size, void* entry, bool usermode);
    };

    API void add_thread(Thread* thread);
    API void remove_thread(Thread* thread);
}
