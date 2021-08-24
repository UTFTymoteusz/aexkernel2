#include "aex/proc/process.hpp"

#include "aex/errno.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/math.hpp"

namespace AEX::Proc {
    using namespace AEX::IPC;

    void Process::ipc_init() {
        m_signals[SIGABRT].action   = SIG_CORE;
        m_signals[SIGALRM].action   = SIG_TERM;
        m_signals[SIGBUS].action    = SIG_CORE;
        m_signals[SIGCHLD].action   = SIG_IGN;
        m_signals[SIGCONT].action   = SIG_CONT;
        m_signals[SIGFPE].action    = SIG_CORE;
        m_signals[SIGHUP].action    = SIG_TERM;
        m_signals[SIGILL].action    = SIG_CORE;
        m_signals[SIGINT].action    = SIG_TERM;
        m_signals[SIGIO].action     = SIG_TERM;
        m_signals[SIGKILL].action   = SIG_TERM;
        m_signals[SIGPIPE].action   = SIG_TERM;
        m_signals[SIGPROF].action   = SIG_TERM;
        m_signals[SIGPWR].action    = SIG_TERM;
        m_signals[SIGQUIT].action   = SIG_CORE;
        m_signals[SIGSEGV].action   = SIG_CORE;
        m_signals[SIGSTKFLT].action = SIG_TERM;
        m_signals[SIGSTOP].action   = SIG_STOP;
        m_signals[SIGTSTP].action   = SIG_STOP;
        m_signals[SIGSYS].action    = SIG_CORE;
        m_signals[SIGTERM].action   = SIG_TERM;
        m_signals[SIGTRAP].action   = SIG_CORE;
        m_signals[SIGTTIN].action   = SIG_STOP;
        m_signals[SIGTTOU].action   = SIG_STOP;
        m_signals[SIGURG].action    = SIG_IGN;
        m_signals[SIGUSR1].action   = SIG_TERM;
        m_signals[SIGUSR2].action   = SIG_TERM;
        m_signals[SIGVTALRM].action = SIG_TERM;
        m_signals[SIGXCPU].action   = SIG_CORE;
        m_signals[SIGXFSZ].action   = SIG_CORE;
        m_signals[SIGWINCH].action  = SIG_IGN;
        m_signals[SIGCNCL].action   = SIG_IGN;
    }

    error_t Process::signal(siginfo_t& info) {
        if (!inrange(info.si_signo, SIGHUP, SIGSYS))
            return EINVAL;

        printkd(PTKD_IPC, "ipc: pid%i: Signal %s\n", this->pid,
                strsignal((signal_t) info.si_signo));
        m_sigqueue.push(info);

        return ENONE;
    }

    optional<sigaction> Process::sigaction(uint8_t id) {
        if (!inrange(id, SIGHUP, SIGSYS) && id != SIGCNCL)
            return {};

        SCOPE(sigact_lock);
        return m_signals[id];
    }

    error_t Process::sigaction(uint8_t id, IPC::sigaction& action) {
        if (!inrange(id, SIGHUP, SIGSYS) && id != SIGCNCL)
            return EINVAL;

        if (id == SIGKILL || id == SIGSTOP || id == SIGCONT)
            return EINVAL;

        SCOPE(sigact_lock);

        if (action.action == SIG_DFL) {
            const sig_action_t def[72] = {
                SIG_IGN,  SIG_TERM, SIG_TERM, SIG_CORE, SIG_CORE, SIG_CORE, SIG_CORE, SIG_CORE,
                SIG_TERM, SIG_TERM, SIG_CORE, SIG_TERM, SIG_TERM, SIG_TERM, SIG_TERM, SIG_IGN,
                SIG_CONT, SIG_STOP, SIG_STOP, SIG_STOP, SIG_STOP, SIG_IGN,  SIG_CORE, SIG_CORE,
                SIG_TERM, SIG_TERM, SIG_IGN,  SIG_TERM, SIG_TERM, SIG_CORE, SIG_IGN,  SIG_IGN,
                SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,
                SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,
                SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,
                SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,
                SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,  SIG_IGN,
            };
            action.action = def[id];
        }

        m_signals[id] = action;
        return ENONE;
    }

    IPC::sigset_t Process::getSignalPending() {
        SCOPE(sigcheck_lock);
        return m_sigqueue.pending();
    }
}