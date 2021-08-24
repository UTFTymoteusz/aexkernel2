#include "aex/proc/thread.hpp"

#include "aex/errno.hpp"
#include "aex/proc/process.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Proc;

void nsleep(uint64_t ns) {
    SCOPE(Thread::current()->signabilityGuard);

    Thread::nsleep(ns);
    USR_ERRNO = ENONE;
}

void yield() {
    Thread::yield();
    USR_ERRNO = ENONE;
}

tid_t tcreate(void* entry, size_t stack_size, void* arg1, void* arg2) {
    auto thread = USR_ENSURE_OPT(
        Proc::Thread::create(Proc::Process::current()->pid, entry, stack_size, nullptr, true));

    thread->setSignalMask(Thread::current()->getSignalMask());
    thread->setArguments(arg1, arg2);
    thread->start();

    return thread->tid;
}

int tjoin(tid_t tid, void** retval) {
    auto joinee   = ENSURE_OPT(Process::current()->get(tid));
    auto retval_k = ENSURE_OPT(joinee->join());

    if (retval)
        usr_write(retval, retval_k);

    return 0;
}

int tkill(tid_t tid, int signal) {
    auto victim = ENSURE_OPT(Process::current()->get(tid));

    if (signal == 0) {
        USR_ERRNO = ENONE;
        return 0;
    }

    IPC::siginfo_t info;

    info.si_signo = signal;
    info.si_uid   = Process::current()->real_uid;
    info.si_code  = SI_USER;

    return victim->signal(info);
}

void texit(void* retval) {
    Thread::exit(retval);
}

__attribute__((optimize("O2"))) void register_proc_thread(Sys::syscall_t* table) {
    table[SYS_NSLEEP]  = (void*) nsleep;
    table[SYS_YIELD]   = (void*) yield;
    table[SYS_TCREATE] = (void*) tcreate;
    table[SYS_TJOIN]   = (void*) tjoin;
    table[SYS_TKILL]   = (void*) tkill;
    table[SYS_TEXIT]   = (void*) texit;
}