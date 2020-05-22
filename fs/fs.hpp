#pragma once

#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"


namespace AEX::FS {
    class Mount;

    void init();

    optional<Mem::SmartPointer<Mount>> find_mount(char* path);
}