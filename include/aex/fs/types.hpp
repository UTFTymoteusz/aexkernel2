#pragma once

#include "aex/dev/types.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/sec/types.hpp"
#include "aex/sys/time/types.hpp"

namespace AEX::FS {
    typedef int64_t blkcnt_t;
    typedef int64_t blksize_t;

    typedef uint64_t fsblkcnt_t;
    typedef uint64_t fsfilcnt_t;

    typedef uint64_t ino_t;
    typedef int32_t  mode_t;
    typedef int64_t  off_t;
    typedef int64_t  nlink_t;

    enum fs_type_t {
        FT_UNKNOWN   = 0x00000000,
        FT_REGULAR   = 0x00010000,
        FT_DIRECTORY = 0x00100000,
        FT_CHAR      = 0x00020000,
        FT_BLOCK     = 0x00060000,
        FT_NET       = 0x000A0000,
        FT_LINK      = 0x00200000,
        FT_FIFO      = 0x00400000,
        FT_MQ        = 0x01000000,
        FT_SEM       = 0x02000000,
        FT_SHM       = 0x04000000,
    };

    struct file_info {
        Dev::dev_t containing_dev_id = -1;
        ino_t      inode;

        fs_type_t type = FT_UNKNOWN;
        mode_t    mode;

        nlink_t hard_links;

        Sec::uid_t uid;
        Sec::gid_t gid;
        Dev::dev_t dev = -1;

        off_t total_size = 0;

        Sys::Time::time_t access_time;
        Sys::Time::time_t modify_time;
        Sys::Time::time_t change_time;

        blksize_t block_size;
        blkcnt_t  blocks;

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

    class Filesystem;
    class Mount;
    class ControlBlock;

    class INode;
    typedef Mem::SmartPointer<INode> INode_SP;

    class File;
    typedef Mem::SmartPointer<File> File_SP;

    class Directory;
    typedef Mem::SmartPointer<Directory> Directory_SP;
}