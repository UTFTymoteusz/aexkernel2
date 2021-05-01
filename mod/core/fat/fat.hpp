#pragma once

#include "aex/fs/controlblock.hpp"
#include "aex/fs/filesystem.hpp"
#include "aex/optional.hpp"

#include "types.hpp"

namespace AEX::FS {
    class FAT : public Filesystem {
        public:
        static void init();

        FAT();
        ~FAT();

        optional<ControlBlock*> mount(const char* source);

        private:
        fat_info gatherInfo(Dev::BlockHandle handle);
    };
}