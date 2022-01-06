#include "aex/proc/thread.hpp"

#include "aex/arch/ipc/signal.hpp"
#include "aex/errno.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/math.hpp"
#include "aex/proc/broker.hpp"
#include "aex/proc/process.hpp"
#include "aex/utility.hpp"

namespace AEX::Proc {
    using namespace IPC;

    extern "C" void proc_ctxload();
    extern "C" void proc_reshed_nosave();

    error_t Thread::signal(siginfo_t& info) {
        if (!inrange(info.si_signo, SIGHUP, SIGSYS) && info.si_signo != SIGCNCL)
            return EINVAL;

        // TODO: change this over to the normal lock
        SCOPE(sigcheck_lock);
        return _signal(info);
    }

    error_t Thread::sigret() {
        AEX_ASSERT(this == Thread::current());

        printkd(PTKD_IPC, "ipc: th%p: Sigret\n", this);
        sigexit();

        return ENONE;
    }

    error_t Thread::_signal(siginfo_t& info) {
        // TODO: change this over to the normal lock
        AEX_ASSERT(sigcheck_lock.isAcquired());

        printkd(PTKD_IPC, "ipc: th%p: Signal %s %s\n", this, strsignal((signal_t) info.si_signo),
                this->m_sigqueue.mask.member(info.si_signo) ? "~masked~" : "");
        m_sigqueue.push(info);

        if (this->isSignalable() && (status == TS_BLOCKED || status == TS_SLEEPING))
            status = TS_RUNNABLE;

        return ENONE;
    }

    error_t Thread::sighandle(siginfo_t& info, state_combo scmb) {
        auto process = getProcess();
        auto handler = process->sigaction(info.si_signo).value;

        switch (handler.action) {
        case SIG_CORE:
            printkd(PTKD_IPC, WARN "ipc: pid%i: Core dump\n", process->pid);
            process->exit(info.si_signo | 0x80);
            break;
        case SIG_TERM:
            printkd(PTKD_IPC, WARN "ipc: pid%i: Termination\n", process->pid);
            process->exit(info.si_signo);
            break;
        case SIG_STOP:
            process->stopped = true;
            break;
        case SIG_CONT:
            process->stopped = false;
            break;
        case SIG_USER:
            AEX_ASSERT(!this->isBusy() || scmb.finfo_regs || scmb.syscall_regs);

            error_t err;
            interruptible(false) {
                err = this->sigenter(info, scmb);
            }

            if (err == EFAULT) {
                printkd(PTKD_IPC, WARN "ipc: pid%i: Core dump (signal stack fault)\n",
                        process->pid);
                process->exit(SIGSEGV | 0x80);
            }
            break;
        default:
            break;
        }

        return ENONE;
    }

    error_t Thread::sigenter(siginfo_t& info, state_combo scmb) {
        auto action = this->getProcess()->sigaction(info.si_signo).value;
        return sigpush(info.si_signo, action, info, scmb);
    }

    error_t Thread::sigexit() {
        interruptible(false) {
            AEX_ASSERT(sigpop() == ENONE);
            proc_ctxload();
        }

        BROKEN;
    }

    bool Thread::sigchk(state_combo scmb) {
        if (m_sigwait)
            return false;

        if (!sigcheck_lock.tryAcquireRaw())
            return false;

        // TODO: Find a better locking mechanism (or perhaps use a RCU-esque solution)
        for (int i = 0; i < this->m_sigqueue.count(); i++) {
            auto info = this->m_sigqueue.peek(i);
            if (this->m_sigqueue.mask.member(info.si_signo))
                continue;

            if (!m_asyncthrdsig && (!scmb.valid() && scmb.syscall_regs) && info.si_signo >= SIGCNCL)
                continue;

            auto action = parent->sigaction(info.si_signo).value.action;
            this->m_sigqueue.erase(i--);

            sighandle(info, scmb);

            if (action == SIG_USER) {
                sigcheck_lock.releaseRaw();
                return true;
            }
        }

        if (parent->sigcheck_lock.tryAcquireRaw()) {
            for (int i = 0; i < parent->m_sigqueue.count(); i++) {
                auto info = parent->m_sigqueue.peek(i);
                if (parent->m_sigqueue.mask.member(info.si_signo) ||
                    this->m_sigqueue.mask.member(info.si_signo))
                    continue;

                auto action = parent->sigaction(info.si_signo).value.action;
                parent->m_sigqueue.erase(i--);

                sighandle(info, scmb);

                if (action == SIG_USER) {
                    parent->sigcheck_lock.releaseRaw();
                    sigcheck_lock.releaseRaw();
                    return true;
                }
            }

            parent->sigcheck_lock.releaseRaw();
        }

        sigcheck_lock.releaseRaw();
        return true;
    }

    void Thread::sigthasync(bool asyncthrdsig) {
        m_asyncthrdsig = asyncthrdsig;
    }

    IPC::sigset_t Thread::sigmask() {
        SCOPE(sigcheck_lock);
        return m_sigqueue.mask;
    }

    void Thread::sigmask(const IPC::sigset_t& mask) {
        printkd(PTKD_IPC, "ipc: thread %i:%i mask set to: ", parent->pid, this->tid);

        for (int i = 0; i < 4; i++)
            printk("0x%08x ", mask.quads[i]);

        printk("\n");

        SCOPE(sigcheck_lock);

        m_sigqueue.mask.set(mask);
        m_sigqueue.recalculate();
    }

    void Thread::sigwait(bool sigwait) {
        m_sigwait = sigwait;
    }

    optional<IPC::siginfo_t> Thread::sigget(IPC::sigset_t* set) {
        SCOPE(this->sigcheck_lock);

        // TODO: Find a better locking mechanism (or perhaps use a RCU-esque solution)
        for (int i = 0; i < this->m_sigqueue.count(); i++) {
            auto info = this->m_sigqueue.peek(i);
            if (this->m_sigqueue.mask.member(info.si_signo))
                continue;

            if (set && !set->member(info.si_signo))
                continue;

            if (!m_asyncthrdsig && info.si_signo >= SIGCNCL)
                continue;

            return this->m_sigqueue.erase(i);
        }

        parent->sigcheck_lock.acquire();

        for (int i = 0; i < parent->m_sigqueue.count(); i++) {
            auto info = parent->m_sigqueue.peek(i);
            if (parent->m_sigqueue.mask.member(info.si_signo))
                continue;

            if (set && !set->member(info.si_signo))
                continue;

            if (!m_asyncthrdsig && info.si_signo >= SIGCNCL)
                continue;

            parent->m_sigqueue.erase(i);

            parent->sigcheck_lock.release();
            return info;
        }

        parent->sigcheck_lock.release();
        return {};
    }

    IPC::sigset_t Thread::sigpending() {
        SCOPE(sigcheck_lock);
        return m_sigqueue.pending();
    }
}