#pragma once

#include "aex/mem/smartarray.hpp"

namespace AEX::Dev {
    class Block;
}

namespace AEX::FS {
    enum type_t {
        UNKNOWN = 0x0000,
        REGULAR = 0x0001,
        CHAR    = 0x0002,
        BLOCK   = 0x0006,
        NET     = 0x000A,
    };

    class Filesystem;
    class Mount;

    extern Mem::SmartArray<Filesystem> filesystems;
    extern Mem::SmartArray<Mount>      mounts;

    int mount(const char* source, const char* path, const char* type);
}