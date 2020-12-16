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
    SYS_PIPE   = 13,
    SYS_FORK   = 14,

    SYS_THCREATE = 15,
    SYS_THJOIN   = 16,
    SYS_THABORT  = 17,
    SYS_THDETACH = 18,

    SYS_TEST1 = 251,
    SYS_TEST2 = 252,
    SYS_TEST3 = 253,
    SYS_TEST4 = 254,
    SYS_TEST5 = 255,
};
