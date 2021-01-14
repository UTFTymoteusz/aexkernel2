#pragma once

#include "aex/math.hpp"
#include "aex/printk.hpp"
#include "aex/proc/types.hpp"

#include <stddef.h>
#include <stdint.h>

#define SA_SIGINFO 0x01
#define SA_RESTORER 0x02

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
            handler  = (void*) act.sa_handler;
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