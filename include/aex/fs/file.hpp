#pragma once

#include "aex/dev/device.hpp"
#include "aex/fs/directory.hpp"
#include "aex/fs/fs.hpp"
#include "aex/fs/path.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    struct file_info {
        Dev::devid_t  containing_dev_id = -1;
        FS::fs_type_t type              = fs_type_t::UNKNOWN;
        Dev::devid_t  dev_id            = -1;
        uint64_t      total_size        = 0;

        bool is_regular() {
            return (type & FS::fs_type_t::REGULAR) == FS::fs_type_t::REGULAR;
        }

        bool is_directory() {
            return (type & FS::fs_type_t::DIRECTORY) == FS::fs_type_t::DIRECTORY;
        }

        bool is_block() {
            return (type & FS::fs_type_t::BLOCK) == FS::fs_type_t::BLOCK;
        }
    };

    class File {
        public:
        enum seek_mode {
            SET     = 0,
            CURRENT = 1,
            END     = 2,
        };

        virtual ~File();

        static optional<Mem::SmartPointer<File>> open(const char* path);
        static optional<Mem::SmartPointer<File>> opendir(const char* path);

        static optional<file_info> info(const char* path);

        virtual optional<uint32_t> read(void* buf, uint32_t count);
        virtual optional<uint32_t> write(void* buf, uint32_t count);

        virtual optional<int64_t> seek(int64_t offset, seek_mode mode = seek_mode::SET);

        virtual optional<dir_entry> readdir();

        virtual void close();

        private:
    };

    typedef Mem::SmartPointer<File> File_SP;
}