#include "aex/errno.hpp"

namespace AEX {
    const char* error_names[64] = {
        "None",
        "Not implemented",
        "No such file or directory",
        "Out of memory",
        "Invalid argument",
        "Interrupted system call",
        "Not a directory",
        "Is a directory",
    };

    const char* strerror(error_t code) {
        if (code < 0 || code >= 64)
            return "Unknown";

        return error_names[code];
    }
}