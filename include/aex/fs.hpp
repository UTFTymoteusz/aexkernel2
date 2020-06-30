#pragma once

#include "aex/errno.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/path.hpp"
#include "aex/fs/type.hpp"
#include "aex/mem/smartarray.hpp"

namespace AEX::Dev {
    class BlockDevice;
}

namespace AEX::FS {
    enum fs_type_t;

    class Filesystem;
    class Mount;

    class File;
    class Directory;

    extern Mem::SmartArray<Filesystem> filesystems;
    extern Mem::SmartArray<Mount>      mounts;

    error_t mount(const char* source, const char* path, const char* type);
}