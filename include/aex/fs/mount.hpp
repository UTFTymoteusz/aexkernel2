#pragma once

#include "aex/errno.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/path.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class Mount {
      public:
        char path[Path::MAX_PATH_LEN];

        virtual ~Mount();

        virtual void unmount();

        virtual optional<Mem::SmartPointer<File>> open(const char* lpath);
        virtual optional<Mem::SmartPointer<File>> opendir(const char* lpath);

        virtual optional<file_info> info(const char* lpath);

      private:
    };
}