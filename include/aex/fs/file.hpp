#pragma once

#include "aex/dev/device.hpp"
#include "aex/fs/fs.hpp"
#include "aex/fs/path.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"

namespace AEX::FS {
    struct dir_entry {
        char       name[Path::MAX_FILENAME_LEN] = {};
        FS::type_t type                         = type_t::UNKNOWN;

        dir_entry(){};
        dir_entry(const char* name);

        bool is_regular() {
            return (type & FS::type_t::REGULAR) == FS::type_t::REGULAR;
        }

        bool is_directory() {
            return (type & FS::type_t::DIRECTORY) == FS::type_t::DIRECTORY;
        }

        bool is_block() {
            return (type & FS::type_t::BLOCK) == FS::type_t::BLOCK;
        }
    };

    struct file_info {
        Dev::devid_t containing_dev_id = -1;
        FS::type_t   type              = type_t::UNKNOWN;
        Dev::devid_t dev_id            = -1;
        uint64_t     total_size        = 0;

        bool is_regular() {
            return (type & FS::type_t::REGULAR) == FS::type_t::REGULAR;
        }

        bool is_directory() {
            return (type & FS::type_t::DIRECTORY) == FS::type_t::DIRECTORY;
        }

        bool is_block() {
            return (type & FS::type_t::BLOCK) == FS::type_t::BLOCK;
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
}