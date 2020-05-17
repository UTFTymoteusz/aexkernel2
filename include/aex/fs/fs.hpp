#pragma once

#include "aex/mem/smartarray.hpp"

namespace AEX::Dev {
    class Block;
}

namespace AEX::FS {
    class Filesystem;
    class Mount;

    extern Mem::SmartArray<Filesystem> filesystems;
    extern Mem::SmartArray<Mount>      mounts;

    int mount(const char* source, const char* path, const char* type);
}