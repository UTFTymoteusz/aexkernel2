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

    error_t Thread::signal(siginfo_t& info) {
        if (!inrange(info.si_signo, SIGHUP, SIGSYS))
            return EINVAL;

        if (Thread::current() == this)
            Thread::current()->addCritical();
        else
            lock.acquire();

        auto err = _signal(info);

        if (Thread::current() == this)
            Thread::current()->subCritical();
        else
            lock.release();

        return err;
    }

    error_t Thread::sigret() {
        AEX_ASSERT(this == Thread::current());

        if (!in_signal)
            return EINVAL;

        exitSignal();

        return ENONE;
    }

    error_t Thread::_signal(siginfo_t& info) {
        if (!inrange(info.si_signo, SIGHUP, SIGSYS))
            return EINVAL;

        if (isBusy()) {
            m_pending_signals.push(info);
            return ENONE;
        }

        // PRINTK_DEBUG2("th0x%p: Signal %i", this, id);
        handleSignal(info);

        return ENONE;
    }

    error_t Thread::handleSignal(IPC::siginfo_t& info) {
        auto process = getProcess();
        auto handler = process->sigaction(info.si_signo).value;

        switch (handler.action) {
        case SIG_CORE:
            PRINTK_DEBUG1("pid%i: Core dump", process->pid);

            process->exit(info.si_signo | 0x80);
            break;
        case SIG_TERM:
            PRINTK_DEBUG1("pid%i: Termination", process->pid);

            process->exit(info.si_signo);
            break;
        case SIG_STOP:
            NOT_IMPLEMENTED;
        case SIG_CONT:
            NOT_IMPLEMENTED;
        case SIG_USER:
            if (Thread::current() != this)
                lock.acquire();

            if (!getBusy() && !in_signal)
                enterSignal(info);
            else
                m_pending_signals.push(info);

            if (Thread::current() != this)
                lock.release();

            break;
        default:
            break;
        }

        return ENONE;
    }

    struct swapdata {
        Thread*   thread;
        siginfo_t info;
        bool      complete;
    };

    void signal_enter(swapdata* data) {
        auto thread = data->thread;
        auto scope  = thread->lock.scope();

        auto info   = data->info;
        auto action = thread->getProcess()->sigaction(info.si_signo).value;

        *thread->context_aux = *thread->context;
        thread->status       = TS_RUNNABLE;

        swap(thread->kernel_stack, thread->aux_stack);
        swap(thread->kernel_stack_size, thread->aux_stack_size);

        if (action.flags & SA_SIGINFO)
            signal_context(thread, info.si_signo, action, info);
        else
            signal_context(thread, info.si_signo, action);

        data->complete = true;
    }

    // TODO: Some sorcery that'll allow me to get rid of that broker call
    error_t Thread::enterSignal(siginfo_t& info) {
        in_signal = true;

        swapdata data = {
            .thread   = this,
            .info     = info,
            .complete = false,
        };

        broker(signal_enter, &data);

        while (!data.complete)
            Proc::Thread::yield();

        return ENONE;
    }

    void signal_exit(Thread* thread) {
        auto scope = thread->lock.scope();

        *thread->context = *thread->context_aux;

        swap(thread->kernel_stack, thread->aux_stack);
        swap(thread->kernel_stack_size, thread->aux_stack_size);

        // As Thread::exitSignal() never returns, we need to subBusy by ourselves
        thread->subBusy();

        thread->in_signal = false;
    }

    // TODO: Some sorcery that'll allow me to get rid of that broker call
    error_t Thread::exitSignal() {
        AEX_ASSERT(Thread::current() == this);

        broker(signal_exit, this);

        while (true)
            Proc::Thread::yield();
    }
}