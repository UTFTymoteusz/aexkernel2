#pragma once

#include "aex/fs/file.hpp"
#include "aex/fs/path.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class Directory : public File {
      public:
        struct entry {
            char name[Path::MAX_FILENAME_LEN];
        };

        virtual ~Directory();

        static optional<Mem::SmartPointer<Directory>> open(const char* path);

      private:
    };
}