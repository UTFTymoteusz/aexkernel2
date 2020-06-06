#pragma once

#include "aex/fs/filesystem.hpp"
#include "aex/fs/mount.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class ISO9660 : public Filesystem {
        public:
        static void init();

        ISO9660();
        ~ISO9660();

        optional<Mount*> mount(const char* source);

        private:
    };
}