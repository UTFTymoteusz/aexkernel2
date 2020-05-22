#pragma once

#include "aex/fs/mount.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    void init();

    optional<Mem::SmartPointer<Mount>> find_mount(const char* path);
}