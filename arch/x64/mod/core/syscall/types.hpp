#pragma once

typedef int dev_t;
typedef int ino_t;

enum perm_test_t {
    R_OK = 0x01,
    W_OK = 0x02,
    X_OK = 0x04,
    F_OK = 0x08,
};

struct stat {
    dev_t st_dev;
    dev_t st_ino;
};