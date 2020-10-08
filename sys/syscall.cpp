#include "aex/sys/syscall.hpp"

#include "aex/errno.hpp"
#include "aex/printk.hpp"

namespace AEX::Sys {
    syscall_t default_tbl[256];

    error_t dummy_syscall();

    void syscall_init() {
        for (int i = 0; i < 256; i++)
            default_tbl[i] = (void*) dummy_syscall;
    }

    syscall_t* default_table() {
        return default_tbl;
    }

    error_t dummy_syscall() {
        printk(PRINTK_WARN "syscall: dummy called\n");
        return ENOSYS;
    }
}