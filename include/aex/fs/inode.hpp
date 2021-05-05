#pragma once

#include "aex/fs/directory.hpp"
#include "aex/fs/types.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"
#include "aex/types.hpp"
#include "aex/utility.hpp"

namespace AEX::FS {
    class API INode {
        public:
        struct cache_entry {
            File* file;

            blk_t    id;
            uint8_t* data;
        };

        Mutex mutex;

        ino_t id;

        fs_type_t type = FT_UNKNOWN;
        mode_t    mode;

        nlink_t hard_links;

        Sec::uid_t uid;
        Sec::gid_t gid;
        Dev::dev_t dev = -1;

        Sys::Time::time_t access_time;
        Sys::Time::time_t modify_time;
        Sys::Time::time_t change_time;

        blkcnt_t  block_count;
        blksize_t block_size;

        off_t size;

        ControlBlock* control_block;

        virtual ~INode();

        virtual error_t readBlocks(void* buffer, blk_t start, blkcnt_t count);
        virtual error_t writeBlocks(const void* buffer, blk_t start, blkcnt_t count);
        virtual error_t update();

        virtual optional<dirent> readDir(dir_context* ctx);
        virtual error_t          seekDir(dir_context* ctx, long pos);
        virtual long             tellDir(dir_context* ctx);

        cache_entry* getCacheEntry(File* file);
        cache_entry* getCacheEntry(blk_t id);
        void         evictCacheEntry(File* file);

        bool is_regular() {
            return (type & FT_REGULAR) == FT_REGULAR;
        }

        bool is_directory() {
            return (type & FT_DIRECTORY) == FT_DIRECTORY;
        }

        bool is_block() {
            return (type & FT_BLOCK) == FT_BLOCK;
        }

        private:
        Mem::Vector<cache_entry*> m_cache;
    };
}