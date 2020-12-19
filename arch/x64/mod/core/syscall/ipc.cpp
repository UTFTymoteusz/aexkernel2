#include "aex/errno.hpp"
#include "aex/ipc/pipe.hpp"
#include "aex/proc.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"
#include "uptr.hpp"

using namespace AEX;

error_t pipe(usr_int* rp, usr_int* wp) {
    auto current = Proc::Process::current();

    FS::File_SP rsp, wsp;

    auto err = IPC::Pipe::create(rsp, wsp);
    if (err != ENONE)
        return err;

    current->files_lock.acquire();

    *rp = current->files.push(rsp);
    *wp = current->files.push(wsp);

    current->files_lock.release();

    return ENONE;
}

void register_ipc() {
    auto table = Sys::default_table();

    table[SYS_PIPE] = (void*) pipe;
}