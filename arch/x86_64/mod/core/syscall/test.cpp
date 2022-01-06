#include "aex/errno.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"
#include "aex/sys/power.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"

#include <stdint.h>

using namespace AEX;

void sys_poweroff() {
    Sys::Power::poweroff();
}

void sys_panic() {
    kpanic("Userspace-triggered kernel panic");
}

error_t sys_test1(uint64_t a) {
    printk("syscall: test1(0x%x)\n", a);
    return ENONE;
}

error_t sys_test2(uint64_t a, uint64_t b) {
    printk("syscall: test2(0x%x, 0x%x)\n", a, b);
    return ENONE;
}

error_t sys_test3(uint64_t a, uint64_t b, uint64_t c) {
    printk("syscall: test3(0x%x, 0x%x, 0x%x)\n", a, b, c);
    return ENONE;
}

error_t sys_test4(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    printk("syscall: test4(0x%x, 0x%x, 0x%x, 0x%x)\n", a, b, c, d);
    return ENONE;
}

error_t sys_test5(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e) {
    printk("syscall: test5(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n", a, b, c, d, e);
    return ENONE;
}

O2 void register_test(Sys::syscall_t* table) {
    table[SYS_POWEROFF] = (void*) sys_poweroff;
    table[SYS_PANIC]    = (void*) sys_panic;
    table[SYS_TEST1]    = (void*) sys_test1;
    table[SYS_TEST2]    = (void*) sys_test2;
    table[SYS_TEST3]    = (void*) sys_test3;
    table[SYS_TEST4]    = (void*) sys_test4;
    table[SYS_TEST5]    = (void*) sys_test5;
}