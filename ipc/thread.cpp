#include "aex/proc/thread.hpp"

#include "aex/arch/ipc/signal.hpp"
#include "aex/arch/mem/helpers/usr_stack.hpp"
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

        SCOPE(sigcheck_lock);
        return _signal(info);
    }

    error_t Thread::sigret() {
        AEX_ASSERT(this == Thread::current());

        printkd(PTKD_IPC, "ipc: th0x%p: Sigret\n", this);
        exitSignal();

        return ENONE;
    }

    error_t Thread::_signal(siginfo_t& info) {
        AEX_ASSERT(sigcheck_lock.isAcquired());

        printkd(PTKD_IPC, "ipc: th0x%p: Signal %s %s\n", this, strsignal((signal_t) info.si_signo),
                this->m_sigqueue.mask.member(info.si_signo) ? "~masked~" : "");
        m_sigqueue.push(info);

        if (this->isSignalable() && (status == TS_BLOCKED || status == TS_SLEEPING))
            setStatus(TS_RUNNABLE);

        return ENONE;
    }

    error_t Thread::handleSignal(siginfo_t& info, IPC::state_combo* combo) {
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
            AEX_ASSERT(!this->isBusy() || this->safe_exit);
            interruptible(false) {
                this->enterSignal(info, combo);
            }

            break;
        default:
            break;
        }

        return ENONE;
    }

    error_t Thread::enterSignal(siginfo_t& info, IPC::state_combo* combo) {
        auto action = this->getProcess()->sigaction(info.si_signo).value;
        pushSignalContext(info.si_signo, action, combo, info);

        if (Thread::current() == this && safe_exit) {
            safe_exit = false;
            proc_ctxload();
        }

        return ENONE;
    }

    error_t Thread::exitSignal() {
        interruptible(false) {
            popSignalContext();
            proc_ctxload();
        }

        while (true)
            ;
    }

    bool Thread::signalCheck(bool allow_handle) {
        return signalCheck(allow_handle, (state_combo*) nullptr);
    }

    bool Thread::signalCheck(bool allow_handle, IPC::interrupt_registers* regs) {
        auto combo = state_combo(regs);
        return signalCheck(allow_handle, &combo);
    }

    bool Thread::signalCheck(bool allow_handle, IPC::syscall_registers* regs) {
        auto combo = state_combo(regs);
        return signalCheck(allow_handle, &combo);
    }

    bool Thread::signalCheck(bool allow_handle, IPC::state_combo* combo) {
        if (m_sigwait)
            return false;

        if (!sigcheck_lock.tryAcquireRaw())
            return false;

        // TODO: Find a better locking mechanism (or perhaps use a RCU-esque solution)
        for (int i = 0; i < this->m_sigqueue.count(); i++) {
            auto info = this->m_sigqueue.peek(i);
            if (this->m_sigqueue.mask.member(info.si_signo))
                continue;

            if (!m_asyncthrdsig && (!combo || !combo->syscall_regs) && info.si_signo >= SIGCNCL)
                continue;

            auto action = parent->sigaction(info.si_signo).value.action;
            if (!allow_handle && action == SIG_USER)
                continue;

            this->m_sigqueue.erase(i--);
            handleSignal(info, combo);

            if (action == SIG_USER)
                return true;
        }

        using(parent->sigcheck_lock) {
            for (int i = 0; i < parent->m_sigqueue.count(); i++) {
                auto info = parent->m_sigqueue.peek(i);
                if (parent->m_sigqueue.mask.member(info.si_signo) ||
                    this->m_sigqueue.mask.member(info.si_signo))
                    continue;

                auto action = parent->sigaction(info.si_signo).value.action;
                if (!allow_handle && action == SIG_USER)
                    continue;

                parent->m_sigqueue.erase(i--);

                if (action == SIG_USER)
                    parent->sigcheck_lock.release();

                handleSignal(info, combo);

                if (action == SIG_USER)
                    return true;
            }
        }

        sigcheck_lock.releaseRaw();
        return true;
    }

    void Thread::asyncThreadingSignals(bool asyncthrdsig) {
        m_asyncthrdsig = asyncthrdsig;
    }

    IPC::sigset_t Thread::getSignalMask() {
        SCOPE(sigcheck_lock);
        return m_sigqueue.mask;
    }

    void Thread::setSignalMask(const IPC::sigset_t& mask) {
        printkd(PTKD_IPC, "ipc: thread %i:%i mask set to: ", parent->pid, this->tid);

        for (int i = 0; i < 4; i++)
            printk("0x%08x ", mask.quads[i]);

        printk("\n");

        SCOPE(sigcheck_lock);

        m_sigqueue.mask.set(mask);
        m_sigqueue.recalculate();
    }

    void Thread::signalWait(bool sigwait) {
        m_sigwait = sigwait;
    }

    optional<IPC::siginfo_t> Thread::signalGet(IPC::sigset_t* set) {
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

        using(parent->sigcheck_lock) {
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
        }

        return {};
    }

    IPC::sigset_t Thread::getSignalPending() {
        SCOPE(sigcheck_lock);
        return m_sigqueue.pending();
    }
}