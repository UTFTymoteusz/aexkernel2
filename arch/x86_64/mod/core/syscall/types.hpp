#pragma once

#include "aex/types.hpp"

#include <stdint.h>

enum perm_test_t {
    R_OK = 0x01,
    W_OK = 0x02,
    X_OK = 0x04,
    F_OK = 0x08,
};

struct stat {
    AEX::Dev::dev_t        st_dev;
    AEX::FS::ino_t         st_ino;
    AEX::FS::mode_t        st_mode;
    AEX::FS::nlink_t       st_nlink;
    AEX::Sec::uid_t        st_uid;
    AEX::Sec::gid_t        st_gid;
    AEX::Dev::dev_t        st_rdev;
    AEX::FS::off_t         st_size;
    AEX::Sys::Time::time_t st_atime;
    AEX::Sys::Time::time_t st_mtime;
    AEX::Sys::Time::time_t st_ctime;
    AEX::FS::blksize_t     st_blksize;
    AEX::FS::blkcnt_t      st_blocks;
};

struct dirent {
    AEX::FS::ino_t d_ino;
    char           d_name[256];
};