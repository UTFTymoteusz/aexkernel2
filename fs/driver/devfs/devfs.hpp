#pragma once

#include "aex/fs/filesystem.hpp"
#include "aex/fs/mount.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class DevFS : public Filesystem {
      public:
        static void init();

        DevFS();
        ~DevFS();

        optional<Mount*> mount(const char* source);

      private:
    };

    class DevFSMount : public Mount {
      public:
        optional<Mem::SmartPointer<File>> opendir(const char* lpath);

      private:
    };
}