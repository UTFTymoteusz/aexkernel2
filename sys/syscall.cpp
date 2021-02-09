#include "aex/sys/syscall.hpp"

#include "aex/errno.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"

namespace AEX::Sys {
    syscall_t default_tbl[256];

    int dummy_syscall();

    void syscall_init() {
        for (int i = 0; i < 256; i++)
            default_tbl[i] = (void*) dummy_syscall;
    }

    syscall_t* default_table() {
        return default_tbl;
    }

    int dummy_syscall() {
        Proc::Thread::current()->errno = ENOSYS;
        return -1;
    }
}