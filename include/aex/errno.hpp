#pragma once

namespace AEX {
    enum error_t {
        ENONE        = 0,
        ENOSYS       = 1,
        ENOENT       = 2,
        ENOMEM       = 3,
        EINVAL       = 4,
        EINTR        = 5,
        ENOTDIR      = 6,
        EISDIR       = 7,
        ENOTBLK      = 8,
        ENOEXEC      = 9,
        ENETDOWN     = 10,
        ENETUNREACH  = 11,
        ENETRESET    = 12,
        ECONNABORTED = 13,
        ECONNRESET   = 14,
        EISCONN      = 15,
        ENOTCONN     = 16,
        ESHUTDOWN    = 17,
        ETIMEDOUT    = 18,
        ECONNREFUSED = 19,
        EHOSTDOWN    = 20,
        EHOSTUNREACH = 21,
    };

    const char* strerror(error_t code);
}