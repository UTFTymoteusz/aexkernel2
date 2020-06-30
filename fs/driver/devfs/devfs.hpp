#pragma once

#include "aex/fs.hpp"
#include "aex/fs/filesystem.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class DevFS : public Filesystem {
        public:
        static void init();

        DevFS();
        ~DevFS();

        optional<ControlBlock*> mount(const char* source);

        private:
    };
}