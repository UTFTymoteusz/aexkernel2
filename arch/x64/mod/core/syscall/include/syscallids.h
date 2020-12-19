#pragma once

enum syscall_id_t {
    SYS_EXIT   = 0,
    SYS_THEXIT = 1,
    SYS_USLEEP = 2,
    SYS_YIELD  = 3,

    SYS_OPEN   = 4,
    SYS_READ   = 5,
    SYS_WRITE  = 6,
    SYS_CLOSE  = 7,
    SYS_TELL   = 8,
    SYS_SEEK   = 9,
    SYS_IOCTL  = 10,
    SYS_ISATTY = 11,
    SYS_DUP    = 12,
    SYS_DUP2   = 13,
    SYS_PIPE   = 14,

    SYS_FORK   = 44,
    SYS_EXECVE = 45,
    SYS_WAIT   = 46,
    SYS_GETPID = 47,

    SYS_THCREATE = 72,
    SYS_THJOIN   = 73,
    SYS_THABORT  = 74,
    SYS_THDETACH = 75,

    SYS_PANIC = 250,
    SYS_TEST1 = 251,
    SYS_TEST2 = 252,
    SYS_TEST3 = 253,
    SYS_TEST4 = 254,
    SYS_TEST5 = 255,
};
