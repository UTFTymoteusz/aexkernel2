#pragma once

#include "aex/optional.hpp"

namespace AEX::Dev {
    class BlockDevice;
}

namespace AEX::FS {
    class Mount;

    class Filesystem {
        public:
        char name[32];

        Filesystem(const char* name);
        virtual ~Filesystem();

        virtual optional<Mount*> mount(const char* source) = 0;

        private:
    };
}