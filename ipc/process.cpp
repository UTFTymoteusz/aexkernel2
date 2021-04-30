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
    }

    error_t Process::signal(siginfo_t& info) {
        if (!inrange(info.si_signo, SIGHUP, SIGSYS))
            return EINVAL;

        auto thl = threads_lock.scope();

        for (int i = 0; i < threads.count(); i++) {
            if (!threads.present(i))
                continue;

            return threads[i]->signal(info);
        }

        return EINVAL;
    }

    optional<sigaction> Process::sigaction(uint8_t id) {
        if (!inrange(id, 1, IPC::SIGSYS))
            return {};

        lock.acquire();
        auto signal = m_signals[id];
        lock.release();

        return signal;
    }

    error_t Process::sigaction(uint8_t id, IPC::sigaction& action) {
        if (!inrange(id, 1, IPC::SIGSYS))
            return EINVAL;

        if (id == SIGKILL || id == SIGSTOP || id == SIGCONT)
            return EINVAL;

        lock.acquire();
        m_signals[id] = action;

        if (action.action == SIG_DFL) {
            const uint8_t def[32] = {
                0,        SIG_TERM, SIG_TERM, SIG_CORE, SIG_CORE, SIG_CORE, SIG_CORE, SIG_CORE,
                SIG_TERM, SIG_TERM, SIG_CORE, SIG_TERM, SIG_TERM, SIG_TERM, SIG_TERM, SIG_IGN,
                SIG_CONT, SIG_STOP, SIG_STOP, SIG_STOP, SIG_STOP, SIG_IGN,  SIG_CORE, SIG_CORE,
                SIG_TERM, SIG_TERM, SIG_IGN,  SIG_TERM, SIG_TERM, SIG_CORE,
            };

            m_signals[id].action = def[id];
        }

        lock.release();

        return ENONE;
    }
}