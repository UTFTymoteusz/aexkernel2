#include "aex/ipc/signal.hpp"

namespace AEX::IPC {
    const char* signal_names[] = {
        "SIGCHK",    "SIGHUP",  "SIGINT",    "SIGQUIT", "SIGILL",   "SIGTRAP", "SIGABRT", "SIGBUS",
        "SIGFPE",    "SIGKILL", "SIGUSR1",   "SIGSEGV", "SIGUSR2",  "SIGPIPE", "SIGALRM", "SIGTERM",
        "SIGSTKFLT", "SIGCHLD", "SIGCONT",   "SIGSTOP", "SIGTSTP",  "SIGTTIN", "SIGTTOU", "SIGURG",
        "SIGXCPU",   "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO",   "SIGPWR",  "SIGSYS",
        "SIGRT0",    "SIGRT1",  "SIGRT2",    "SIGRT3",  "SIGRT4",   "SIGRT5",  "SIGRT6",  "SIGRT7",
        "SIGRT8",    "SIGRT9",  "SIGRT10",   "SIGRT11", "SIGRT12",  "SIGRT13", "SIGRT14", "SIGRT15",
        "SIGRT16",   "SIGRT17", "SIGRT18",   "SIGRT19", "SIGRT20",  "SIGRT21", "SIGRT22", "SIGRT23",
        "SIGRT24",   "SIGRT25", "SIGRT26",   "SIGRT27", "SIGRT28",  "SIGRT29", "SIGRT30", "SIGRT31",
        "SIGCNCL",   "Invalid", "Invalid",   "Invalid", "Invalid",  "Invalid", "Invalid", "Invalid",
    };

    const char* strsignal(signal_t signal) {
        if (signal < 0 || signal > SIGCNCL)
            signal = SIGINV;

        return signal_names[signal];
    }

    sigaction_usr::sigaction_usr(const sigaction& act) {
        if (act.flags & SA_SIGINFO)
            sa_sigaction = (void (*)(int, siginfo_t*, void*)) act.handler;
        else
            sa_handler = (void (*)(int)) act.handler;

        sa_mask  = act.mask;
        sa_flags = act.flags;
    }

    sigaction::sigaction(const sigaction_usr& act) {
        size_t action_pre = (size_t) (act.sa_flags & SA_SIGINFO ? (void*) act.sa_sigaction
                                                                : (void*) act.sa_handler);

        action   = (sig_action_t) clamp<size_t>(action_pre, SIG_TERM, SIG_USER);
        handler  = (void*) action_pre;
        restorer = (void*) act.sa_restorer;
        mask     = act.sa_mask;
        flags    = act.sa_flags;
    }
}