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
    SYS_CHDIR  = 15,
    SYS_GETCWD = 16,
    SYS_ACCESS = 17,
    SYS_STAT   = 18,
    SYS_MMAP   = 19,
    SYS_MUNMAP = 20,

    SYS_FORK   = 44,
    SYS_EXECVE = 45,
    SYS_WAIT   = 46,
    SYS_GETPID = 47,
    SYS_GETTID = 48,

    SYS_TCREATE = 72,
    SYS_TJOIN   = 73,
    SYS_TABORT  = 74,
    SYS_TDETACH = 75,

    SYS_SIGRET = 94,

    SYS_PANIC = 250,
    SYS_TEST1 = 251,
    SYS_TEST2 = 252,
    SYS_TEST3 = 253,
    SYS_TEST4 = 254,
    SYS_TEST5 = 255,
};
