#pragma once

#include "aex/math.hpp"
#include "aex/printk.hpp"
#include "aex/proc/types.hpp"

#include <stddef.h>
#include <stdint.h>

#define SA_SIGINFO 0x01
#define SA_RESTORER 0x02

#define ILL_ILLOPC 0x00 // Illegal opcode.
#define ILL_ILLOPN 0x01 // Illegal operand.
#define ILL_ILLADR 0x02 // Illegal addressing mode.
#define ILL_ILLTRP 0x03 // Illegal trap.
#define ILL_PRVOPC 0x04 // Privileged opcode.
#define ILL_PRVREG 0x05 // Privileged register.
#define ILL_COPROC 0x06 // Coprocessor error.
#define ILL_BADSTK 0x07 // Internal stack error.

#define FPE_INTDIV 0x00 // Integer divide by zero.
#define FPE_INTOVF 0x01 // Integer overflow.
#define FPE_FLTDIV 0x02 // Floating-point divide by zero.
#define FPE_FLTOVF 0x03 // Floating-point overflow.
#define FPE_FLTUND 0x04 // Floating-point underflow.
#define FPE_FLTRES 0x05 // Floating-point inexact result.
#define FPE_FLTINV 0x06 // Invalid floating-point operation.
#define FPE_FLTSUB 0x07 // Subscript out of range.

#define SEGV_MAPERR 0x00 // Address not mapped to object.
#define SEGV_ACCERR 0x01 // Invalid permissions for mapped object.

#define BUS_ADRALN 0x00 // Invalid address alignment.
#define BUS_ADRERR 0x01 // Nonexistent physical address.
#define BUS_OBJERR 0x02 // Object-specific hardware error.

#define TRAP_BRKPT 0x00 // Process breakpoint.
#define TRAP_TRACE 0x01 // Process trace trap.

#define CLD_EXITED 0x00    // Child has exited.
#define CLD_KILLED 0x01    // Child has terminated abnormally and did not create a core file.
#define CLD_DUMPED 0x02    // Child has terminated abnormally and created a core file.
#define CLD_TRAPPED 0x03   // Traced child has trapped.
#define CLD_STOPPED 0x04   // Child has stopped.
#define CLD_CONTINUED 0x05 // Stopped child has continued.

#define POLL_IN 0x00  // Data input available.
#define POLL_OUT 0x01 // Output buffers available.
#define POLL_MSG 0x02 // Input message available.
#define POLL_ERR 0x03 // I/O error.
#define POLL_PRI 0x04 // High priority input available.
#define POLL_HUP 0x05 // Device disconnected.

#define SI_USER 0x00    // Signal sent by kill().
#define SI_QUEUE 0x01   // Signal sent by sigqueue().
#define SI_TIMER 0x02   // Signal generated by expiration of a timer set by timer_settime().
#define SI_ASYNCIO 0x03 // Signal generated by completion of an asynchronous I/O.
#define SI_MESGQ 0x04   // Signal generated by arrival of a message on an empty message queue.

namespace AEX::IPC {
    typedef uint32_t sigset_t;

    enum signal_t {
        SIGHUP    = 1,
        SIGINT    = 2,
        SIGQUIT   = 3,
        SIGILL    = 4,
        SIGTRAP   = 5,
        SIGABRT   = 6,
        SIGBUS    = 7,
        SIGFPE    = 8,
        SIGKILL   = 9,
        SIGUSR1   = 10,
        SIGSEGV   = 11,
        SIGUSR2   = 12,
        SIGPIPE   = 13,
        SIGALRM   = 14,
        SIGTERM   = 15,
        SIGSTKFLT = 16,
        SIGCHLD   = 17,
        SIGCONT   = 18,
        SIGSTOP   = 19,
        SIGTSTP   = 20,
        SIGTTIN   = 21,
        SIGTTOU   = 22,
        SIGURG    = 23,
        SIGXCPU   = 24,
        SIGXFSZ   = 25,
        SIGVTALRM = 26,
        SIGPROF   = 27,
        SIGWINCH  = 28,
        SIGIO     = 29,
        SIGPWR    = 30,
        SIGSYS    = 31,
    };

    enum sig_action_t {
        SIG_TERM = 0,
        SIG_IGN,
        SIG_CORE,
        SIG_STOP,
        SIG_CONT,
        SIG_DFL,
        SIG_USER,
    };

    union sigval {
        int   sigval_int;
        void* sigval_ptr;
    };

    struct siginfo_t {
        int si_signo;
        int si_code;

        int si_errno;

        Proc::pid_t si_pid;
        // uid_t si_uid; TODO: Make this a reality

        void*  si_addr;
        int    si_status;
        long   si_band;
        sigval si_value;
    };

    struct sigaction_usr {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t*, void*);
        sigset_t sa_mask;
        int      sa_flags;
        void (*sa_restorer)();
    };

    struct sigaction {
        sigaction() {}
        sigaction(const sigaction_usr& act) {
            size_t action_pre = (size_t)(act.sa_flags & SA_SIGINFO ? (void*) act.sa_sigaction
                                                                   : (void*) act.sa_handler);

            action   = clamp<size_t>(action_pre, SIG_TERM, SIG_USER);
            handler  = (void*) action_pre;
            restorer = (void*) act.sa_restorer;
            mask     = act.sa_mask;
            flags    = act.sa_flags;
        }

        uint8_t action;

        void*    handler;
        void*    restorer;
        sigset_t mask;
        int      flags;
    };
}