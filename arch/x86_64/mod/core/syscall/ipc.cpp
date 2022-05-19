#include "aex/errno.hpp"
#include "aex/ipc/pipe.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/mem/usr.hpp"
#include "aex/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/utility.hpp"

#include "syscallids.h"
#include "types.hpp"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Mem;

int sys_pipe(usr_int* rp, usr_int* wp) {
    FS::File_SP rsp, wsp;
    auto        current = Proc::Process::current();

    auto err = IPC::Pipe::create(rsp, wsp);
    if (err != ENONE)
        return err;

    current->descs_mutex.acquire();

    int rfd = current->descs.push(rsp);
    int wfd = current->descs.push(wsp);

    current->descs_mutex.release();

    USR_ENSURE_OPT(u_write(rp, rfd));
    USR_ENSURE_OPT(u_write(wp, wfd));

    return 0;
}

int sys_kill(Proc::pid_t pid, int sig) {
    if (inrange(pid, 1, Proc::Process::kernel()->pid))
        return EPERM;

    USR_ERRNO = Proc::Process::kill(pid, sig);
    return USR_ERRNO ? -1 : 0;
}

int sys_sigqueue(Proc::pid_t pid, int sig, IPC::sigval val) {
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

int sys_sigact(int signum, const IPC::sigaction_usr* act, IPC::sigaction_usr* oldact) {
    auto current = Proc::Process::current();

    if (oldact) {
        IPC::sigaction_usr action = USR_ENSURE_OPT(current->sigaction(signum));
        USR_ENSURE_OPT(u_write(oldact, action));
    }

    if (act) {
        IPC::sigaction action = USR_ENSURE_OPT(u_read(act));

        USR_ENSURE_R((size_t) action.handler < Proc::Process::current()->pagemap->vend, EFAULT);
        USR_ENSURE_ENONE(current->sigaction(signum, action));
    }

    return 0;
}

void sys_sigret() {
    Proc::Thread::current()->sigret();
}

void sys_sigathrd(bool asyncthrdsig) {
    Proc::Thread::current()->sigthasync(asyncthrdsig);
}

int sys_sigpmask(int how, const IPC::sigset_t* usr_set, IPC::sigset_t* usr_oldset) {
    USR_ERRNO = ENOSYS;
    return 0;

    /*auto oldset = Proc::Process::current()->sigmask();
    USR_ERRNO   = ENONE;

    if (usr_oldset)
        USR_ENSURE_OPT(u_write(usr_oldset, oldset));

    if (usr_set) {
        auto set = USR_ENSURE_OPT(u_read(usr_set));
        switch (how) {
        case SIG_BLOCK:
            Proc::Process::current()->sigmask(oldset.block(set));
            break;
        case SIG_UNBLOCK:
            Proc::Process::current()->sigmask(oldset.unblock(set));
            break;
        case SIG_SETMASK:
            Proc::Process::current()->sigmask(set);
            break;
        default:
            USR_ERRNO = EINVAL;
            return -1;
        }
    }

    return 0;*/
}

int sys_sigtmask(int how, const IPC::sigset_t* usr_set, IPC::sigset_t* usr_oldset) {
    auto oldset = Proc::Thread::current()->sigmask();
    USR_ERRNO   = ENONE;

    if (usr_oldset)
        USR_ENSURE_OPT(u_write(usr_oldset, oldset));

    if (usr_set) {
        auto set = USR_ENSURE_OPT(u_read(usr_set));
        switch (how) {
        case SIG_BLOCK:
            Proc::Thread::current()->sigmask(oldset.block(set));
            break;
        case SIG_UNBLOCK:
            Proc::Thread::current()->sigmask(oldset.unblock(set));
            break;
        case SIG_SETMASK:
            Proc::Thread::current()->sigmask(set);
            break;
        default:
            USR_ERRNO = EINVAL;
            return -1;
        }
    }

    return 0;
}

int sys_sigwait(const IPC::sigset_t* usr_set, IPC::siginfo_t* usr_info, uint64_t timeout_ns) {
    auto set    = USR_ENSURE_OPT(u_read(usr_set));
    auto oldset = Proc::Thread::current()->sigmask();

    int          signo = -1;
    AEX::error_t errno = EINTR;

    Proc::Thread::current()->sigmask(Proc::Thread::current()->sigmask().unblock(set));
    Proc::Thread::current()->sigwait(true);

    using(Proc::Thread::current()->signabilityGuard) {
        // TODO: add an instant check here

        if (timeout_ns)
            Proc::Thread::nsleep(timeout_ns);
        else
            Proc::Thread::current()->status = Proc::TS_BLOCKED;

        Proc::Thread::yield();

        if (Proc::Thread::current()->status == Proc::TS_RUNNABLE && timeout_ns > 0)
            errno = EAGAIN;

        Proc::Thread::current()->status = Proc::TS_RUNNABLE;

        auto info_try = Proc::Thread::current()->sigget(&set);
        if (info_try.has_value) {
            auto info = info_try.value;
            signo     = info.si_signo;

            if (usr_info)
                USR_ENSURE_OPT(u_write(usr_info, info));
        }
    }

    Proc::Thread::current()->sigwait(false);
    Proc::Thread::current()->sigmask(oldset);

    USR_ERRNO = signo == -1 ? errno : ENONE;
    return signo;
}

int sys_sigpending(IPC::sigset_t* usr_set) {
    IPC::sigset_t set = {};

    set.block(Proc::Process::current()->sigpending());
    set.block(Proc::Thread::current()->sigpending());

    if (usr_set)
        USR_ENSURE_OPT(u_write(usr_set, set));

    return 0;
}

int sys_sigsuspend(const IPC::sigset_t* usr_mask) {
    const auto mask    = USR_ENSURE_OPT(u_read(usr_mask));
    auto       oldmask = Proc::Thread::current()->sigmask();

    Proc::Thread::current()->sigmask(mask);

    using(Proc::Thread::current()->signabilityGuard) {
        Proc::Thread::current()->status = Proc::TS_BLOCKED;
        Proc::Thread::yield();
        Proc::Thread::current()->status = Proc::TS_RUNNABLE;
    }

    USR_ERRNO = EINTR;
    return -1;
}

O0 void register_ipc(Sys::syscall_t* table) {
    table[SYS_PIPE]       = (void*) sys_pipe;
    table[SYS_KILL]       = (void*) sys_kill;
    table[SYS_SIGACT]     = (void*) sys_sigact;
    table[SYS_SIGRET]     = (void*) sys_sigret;
    table[SYS_SIGATHRD]   = (void*) sys_sigathrd;
    table[SYS_SIGPMASK]   = (void*) sys_sigpmask;
    table[SYS_SIGTMASK]   = (void*) sys_sigtmask;
    table[SYS_SIGWAIT]    = (void*) sys_sigwait;
    table[SYS_SIGQUEUE]   = (void*) sys_sigqueue;
    table[SYS_SIGPENDING] = (void*) sys_sigpending;
    table[SYS_SIGSUSPEND] = (void*) sys_sigsuspend;
}