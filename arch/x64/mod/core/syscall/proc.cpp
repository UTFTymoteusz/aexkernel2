#include "aex/errno.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"

using namespace AEX;

error_t exit(int status) {
    Proc::Process::current()->exit(status);
    return ENONE;
}

error_t thexit() {
    Proc::Thread::exit();
    return ENONE;
}

error_t usleep(int ns) {
    Proc::Thread::usleep(ns);
    return ENONE;
}

error_t yield() {
    Proc::Thread::yield();
    return ENONE;
}

void register_proc() {
    auto table = Sys::default_table();

    table[SYS_EXIT]   = (void*) exit;
    table[SYS_THEXIT] = (void*) thexit;
    table[SYS_USLEEP] = (void*) usleep;
    table[SYS_YIELD]  = (void*) yield;
}