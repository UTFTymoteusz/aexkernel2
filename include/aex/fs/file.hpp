#pragma once

#include "aex/fs/path.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    class File {
      public:
        struct dir_entry {
            char name[Path::MAX_FILENAME_LEN] = {};

            dir_entry(){};
            dir_entry(const char* name);
        };

        virtual ~File();

        static optional<Mem::SmartPointer<File>> open(const char* path);
        static optional<Mem::SmartPointer<File>> opendir(const char* path);

        virtual optional<dir_entry> readdir();

      private:
    };
}