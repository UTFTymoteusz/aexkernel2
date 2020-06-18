#pragma once

#include "aex/fs/fs.hpp"
#include "aex/fs/path.hpp"

namespace AEX::FS {
    struct dir_entry {
        int inode_id;

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

    struct dir_context {
        int pos;
    };
}