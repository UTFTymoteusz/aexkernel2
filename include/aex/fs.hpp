#pragma once

#include "aex/errno.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/path.hpp"
#include "aex/fs/types.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/utility.hpp"

namespace AEX::Dev {
    class BlockDevice;
}

namespace AEX::FS {
    enum fs_type_t;

    API extern Mem::SmartArray<Filesystem> filesystems;
    API extern Mem::SmartArray<Mount>      mounts;

    API error_t mount(const char* source, const char* path, const char* type);
}