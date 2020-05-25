#pragma once

namespace AEX {
    enum error_t {
        ENONE   = 0,
        ENOSYS  = 1,
        ENOENT  = 2,
        ENOMEM  = 3,
        EINVAL  = 4,
        EINTR   = 5,
        ENOTDIR = 6,
        EISDIR  = 7,
        ENOTBLK = 8,
    };

    const char* strerror(error_t code);
}