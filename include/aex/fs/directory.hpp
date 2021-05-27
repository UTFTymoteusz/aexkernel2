#pragma once

#include "aex/fs/path.hpp"
#include "aex/fs/types.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    struct API dirent {
        ino_t     inode_id;
        int       pos;
        char      name[NAME_MAX] = {};
        fs_type_t type           = FT_UNKNOWN;

        dirent(){};
        dirent(const char* name, int pos, int inode_id);

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

    struct API dir_context {
        off_t pos = 0;
    };
}