#include "aex/errno.hpp"

namespace AEX {
    const char* error_names[64] = {
        [ENONE]   = "None",
        [ENOSYS]  = "Not implemented",
        [ENOENT]  = "No such file or directory",
        [ENOMEM]  = "Out of memory",
        [EINVAL]  = "Invalid argument",
        [EINTR]   = "Interrupted system call",
        [ENOTDIR] = "Not a directory",
        [EISDIR]  = "Is a directory",
        [ENOTBLK] = "Block device required",
        [ENOEXEC] = "Exec format error",
        [EROFS]   = "Read-only filesystem",
        [EBADF]   = "Bad file number",

        [EPROTOTYPE]      = "Protocol wrong type for socket",
        [ENOPROTOOPT]     = "Protocol not available",
        [EPROTONOSUPPORT] = "Protocol not supported",
        [ESOCKTNOSUPPORT] = "Socket type not supported",

        [EADDRINUSE]    = "Address already in use",
        [EADDRNOTAVAIL] = "Cannot assign requested address",
        [ENETDOWN]      = "Network is down",
        [ENETUNREACH]   = "Network is unreachable",
        [ENETRESET]     = "Network dropped connection because of reset",
        [ECONNABORTED]  = "Software caused connection abort",
        [ECONNRESET]    = "Connection reset by peer",
        [EISCONN]       = "Transport endpoint is already connected",
        [ENOTCONN]      = "Transport endpoint is not connected",
        [ESHUTDOWN]     = "Cannot send after transport endpoint shutdown",
        [ETIMEDOUT]     = "Connection timed out",
        [ECONNREFUSED]  = "Connection refused",
        [EHOSTDOWN]     = "Host is down",
        [EHOSTUNREACH]  = "No route to host",

        [EBOTHER] = "Cannot be bothered to implement",
    };

    const char* strerror(error_t code) {
        if (code < 0 || code >= 64)
            return "Unknown";

        return error_names[code];
    }
}