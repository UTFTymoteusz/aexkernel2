#include "aex/errno.hpp"
#include "aex/ipc/pipe.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/proc.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/utility.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;

int pipe(usr_int* rp, usr_int* wp) {
    auto current = Proc::Process::current();

    FS::File_SP rsp, wsp;

    auto err = IPC::Pipe::create(rsp, wsp);
    if (err != ENONE)
        return err;

    current->descs_lock.acquire();

    int rfd = current->descs.push(rsp);
    int wfd = current->descs.push(wsp);

    current->descs_lock.release();

    USR_ENSURE_OPT(usr_write(rp, rfd));
    USR_ENSURE_OPT(usr_write(wp, wfd));

    return 0;
}

int kill(Proc::pid_t pid, int sig) {
    if (inrange(pid, 1, Proc::Process::kernel()->pid))
        return EPERM;

    if (pid == 0)
        NOT_IMPLEMENTED;

    USR_ERRNO = Proc::Process::kill(pid, sig);
    return USR_ERRNO ? -1 : 0;
}

int sigact(int signum, const IPC::sigaction_usr* act, IPC::sigaction_usr* oldact) {
    auto current = Proc::Process::current();

    if (oldact) {
        auto action = USR_ENSURE_OPT(current->sigaction(signum));
        USR_ENSURE_OPT(usr_write(oldact, action));
    }

    if (act) {
        IPC::sigaction action = USR_ENSURE_OPT(usr_read(act));
        USR_ENSURE_ENONE(current->sigaction(signum, action));
    }

    return 0;
}

void sigret() {
    Proc::Thread::current()->sigret();
}

void register_ipc() {
    auto table = Sys::default_table();

    table[SYS_PIPE]   = (void*) pipe;
    table[SYS_KILL]   = (void*) kill;
    table[SYS_SIGACT] = (void*) sigact;
    table[SYS_SIGRET] = (void*) sigret;
}