#pragma once

#include "aex/fs/types.hpp"
#include "aex/optional.hpp"

namespace AEX::Dev {
    class BlockDevice;
}

namespace AEX::FS {
    class Filesystem {
        public:
        char name[32];

        Filesystem(const char* name);
        virtual ~Filesystem();

        virtual optional<ControlBlock*> mount(const char* source) = 0;
    };
}