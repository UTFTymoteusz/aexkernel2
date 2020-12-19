#include "aex/sys/syscall.hpp"

#include "aex/errno.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"

namespace AEX::Sys {
    syscall_t default_tbl[256];

    void dummy_syscall();

    void syscall_init() {
        for (int i = 0; i < 256; i++)
            default_tbl[i] = (void*) dummy_syscall;
    }

    syscall_t* default_table() {
        return default_tbl;
    }

    void dummy_syscall() {
        printk(PRINTK_WARN "syscall: dummy called\n");
        Proc::Thread::current()->errno = ENOSYS;
    }
}