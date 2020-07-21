#pragma once

#include "aex/dev/device.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/type.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    struct file_info {
        Dev::devid_t  containing_dev_id = -1;
        FS::fs_type_t type              = FILE_UNKNOWN;
        Dev::devid_t  dev_id            = -1;
        uint64_t      total_size        = 0;

        bool is_regular() {
            return (type & FS::FILE_REGULAR) == FS::FILE_REGULAR;
        }

        bool is_directory() {
            return (type & FS::FILE_DIRECTORY) == FS::FILE_DIRECTORY;
        }

        bool is_block() {
            return (type & FS::FILE_BLOCK) == FS::FILE_BLOCK;
        }
    };

    class File;
    typedef Mem::SmartPointer<File> File_SP;

    class File {
        public:
        enum seek_mode {
            SET     = 0,
            CURRENT = 1,
            END     = 2,
        };

        virtual ~File();

        static optional<File_SP> open(const char* path);
        static optional<File_SP> opendir(const char* path);

        static optional<file_info> info(const char* path);

        virtual optional<uint32_t> read(void* buf, uint32_t count);
        virtual optional<uint32_t> write(void* buf, uint32_t count);

        virtual optional<int64_t> seek(int64_t offset, seek_mode mode = seek_mode::SET);

        virtual optional<dir_entry> readdir();

        virtual optional<File_SP> dup();

        virtual void close();

        private:
    };
}