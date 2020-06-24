#pragma once

#include "aex/errno.hpp"
#include "aex/mem/smartarray.hpp"

namespace AEX::Dev {
    class BlockDevice;
}

namespace AEX::FS {
    enum fs_type_t {
        UNKNOWN   = 0x0000,
        REGULAR   = 0x0001,
        DIRECTORY = 0x0010,
        CHAR      = 0x0002,
        BLOCK     = 0x0006,
        NET       = 0x000A,
    };

    class Filesystem;
    class Mount;

    extern Mem::SmartArray<Filesystem> filesystems;
    extern Mem::SmartArray<Mount>      mounts;

    error_t mount(const char* source, const char* path, const char* type);
}