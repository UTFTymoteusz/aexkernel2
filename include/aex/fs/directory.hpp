#pragma once

#include "aex/fs/path.hpp"
#include "aex/fs/type.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    struct dir_entry {
        int inode_id;
        int pos;

        char          name[Path::MAX_FILENAME_LEN] = {};
        FS::fs_type_t type                         = FT_UNKNOWN;

        dir_entry(){};
        dir_entry(const char* name, int pos, int inode_id);

        bool is_regular() {
            return (type & FS::FT_REGULAR) == FS::FT_REGULAR;
        }

        bool is_directory() {
            return (type & FS::FT_DIRECTORY) == FS::FT_DIRECTORY;
        }

        bool is_block() {
            return (type & FS::FT_BLOCK) == FS::FT_BLOCK;
        }
    };

    struct dir_context {
        uint64_t pos = 0;
    };
}