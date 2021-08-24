#include "aex/errno.hpp"
#include "aex/ipc/pipe.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/utility.hpp"

#include "syscallids.h"
#include "types.hpp"
#include "usr.hpp"

using namespace AEX;

int pipe(usr_int* rp, usr_int* wp) {
    FS::File_SP rsp, wsp;
    auto        current = Proc::Process::current();

    auto err = IPC::Pipe::create(rsp, wsp);
    if (err != ENONE)
        return err;

    current->descs_mutex.acquire();

    int rfd = current->descs.push(rsp);
    int wfd = current->descs.push(wsp);

    current->descs_mutex.release();

    USR_ENSURE_OPT(usr_write(rp, rfd));
    USR_ENSURE_OPT(usr_write(wp, wfd));

    return 0;
}

int kill(Proc::pid_t pid, int sig) {
    if (inrange(pid, 1, Proc::Process::kernel()->pid))
        return EPERM;

    USR_ERRNO = Proc::Process::kill(pid, sig);
    return USR_ERRNO ? -1 : 0;
}

int sigqueue(Proc::pid_t pid, int sig, IPC::sigval val) {
    if (inrange(pid, 1, Proc::Process::kernel()->pid))
        return EPERM;

    IPC::siginfo_t info;

    info.si_signo = sig;
    info.si_uid   = Proc::Process::current()->real_uid;
    info.si_code  = SI_QUEUE;
    info.si_value = val;

    USR_ERRNO = Proc::Process::kill(pid, info);
    return USR_ERRNO ? -1 : 0;
}

int sigact(int signum, const IPC::sigaction_usr* act, IPC::sigaction_usr* oldact) {
    auto current = Proc::Process::current();

    if (oldact) {
        IPC::sigaction_usr action = USR_ENSURE_OPT(current->sigaction(signum));
        USR_ENSURE_OPT(usr_write(oldact, action));
    }

    if (act) {
        IPC::sigaction action = USR_ENSURE_OPT(usr_read(act));

        USR_ENSURE_R((size_t) action.handler < Proc::Process::current()->pagemap->vend, EFAULT);
        USR_ENSURE_ENONE(current->sigaction(signum, action));
    }

    return 0;
}

void sigret() {
    Proc::Thread::current()->sigret();
}

void sigathrd(bool asyncthrdsig) {
    Proc::Thread::current()->asyncThreadingSignals(asyncthrdsig);
}

int sigpmask(int how, const IPC::sigset_t* usr_set, IPC::sigset_t* usr_oldset) {
    USR_ERRNO = ENOSYS;
    return 0;

    /*auto oldset = Proc::Process::current()->getSignalMask();
    USR_ERRNO   = ENONE;

    if (usr_oldset)
        USR_ENSURE_OPT(usr_write(usr_oldset, oldset));

    if (usr_set) {
        auto set = USR_ENSURE_OPT(usr_read(usr_set));
        switch (how) {
        case SIG_BLOCK:
            Proc::Process::current()->setSignalMask(oldset.block(set));
            break;
        case SIG_UNBLOCK:
            Proc::Process::current()->setSignalMask(oldset.unblock(set));
            break;
        case SIG_SETMASK:
            Proc::Process::current()->setSignalMask(set);
            break;
        default:
            USR_ERRNO = EINVAL;
            return -1;
        }
    }

    return 0;*/
}

int sigtmask(int how, const IPC::sigset_t* usr_set, IPC::sigset_t* usr_oldset) {
    auto oldset = Proc::Thread::current()->getSignalMask();
    USR_ERRNO   = ENONE;

    if (usr_oldset)
        USR_ENSURE_OPT(usr_write(usr_oldset, oldset));

    if (usr_set) {
        auto set = USR_ENSURE_OPT(usr_read(usr_set));
        switch (how) {
        case SIG_BLOCK:
            Proc::Thread::current()->setSignalMask(oldset.block(set));
            break;
        case SIG_UNBLOCK:
            Proc::Thread::current()->setSignalMask(oldset.unblock(set));
            break;
        case SIG_SETMASK:
            Proc::Thread::current()->setSignalMask(set);
            break;
        default:
            USR_ERRNO = EINVAL;
            return -1;
        }
    }

    return 0;
}

int sigwait(const IPC::sigset_t* usr_set, IPC::siginfo_t* usr_info, uint64_t timeout_ns) {
    auto set    = USR_ENSURE_OPT(usr_read(usr_set));
    auto oldset = Proc::Thread::current()->getSignalMask();

    int          signo = -1;
    AEX::error_t errno = EINTR;

    Proc::Thread::current()->setSignalMask(Proc::Thread::current()->getSignalMask().unblock(set));
    Proc::Thread::current()->signalWait(true);

    using(Proc::Thread::current()->signabilityGuard) {
        // TODO: add an instant check here

        if (timeout_ns)
            Proc::Thread::nsleep(timeout_ns);
        else
            Proc::Thread::current()->setStatus(Proc::TS_BLOCKED);

        Proc::Thread::yield();

        if (Proc::Thread::current()->status == Proc::TS_RUNNABLE && timeout_ns > 0)
            errno = EAGAIN;

        Proc::Thread::current()->setStatus(Proc::TS_RUNNABLE);

        auto info_try = Proc::Thread::current()->signalGet(&set);
        if (info_try.has_value) {
            auto info = info_try.value;
            signo     = info.si_signo;

            if (usr_info)
                USR_ENSURE_OPT(usr_write(usr_info, info));
        }
    }

    Proc::Thread::current()->signalWait(false);
    Proc::Thread::current()->setSignalMask(oldset);

    USR_ERRNO = signo == -1 ? errno : ENONE;
    return signo;
}

int sigpending(IPC::sigset_t* usr_set) {
    IPC::sigset_t set = {};

    set.block(Proc::Process::current()->getSignalPending());
    set.block(Proc::Thread::current()->getSignalPending());

    if (usr_set)
        USR_ENSURE_OPT(usr_write(usr_set, set));

    return 0;
}

int sigsuspend(const IPC::sigset_t* usr_mask) {
    const auto mask    = USR_ENSURE_OPT(usr_read(usr_mask));
    auto       oldmask = Proc::Thread::current()->getSignalMask();

    Proc::Thread::current()->setSignalMask(mask);

    using(Proc::Thread::current()->signabilityGuard) {
        Proc::Thread::current()->setStatus(Proc::TS_BLOCKED);
        Proc::Thread::yield();
        Proc::Thread::current()->setStatus(Proc::TS_RUNNABLE);
    }

    USR_ERRNO = EINTR;
    return -1;
}

__attribute__((optimize("O2"))) void register_ipc(Sys::syscall_t* table) {
    table[SYS_PIPE]       = (void*) pipe;
    table[SYS_KILL]       = (void*) kill;
    table[SYS_SIGACT]     = (void*) sigact;
    table[SYS_SIGRET]     = (void*) sigret;
    table[SYS_SIGATHRD]   = (void*) sigathrd;
    table[SYS_SIGPMASK]   = (void*) sigpmask;
    table[SYS_SIGTMASK]   = (void*) sigtmask;
    table[SYS_SIGWAIT]    = (void*) sigwait;
    table[SYS_SIGQUEUE]   = (void*) sigqueue;
    table[SYS_SIGPENDING] = (void*) sigpending;
    table[SYS_SIGSUSPEND] = (void*) sigsuspend;
}