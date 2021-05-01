#pragma once

#include "aex/fs.hpp"
#include "aex/fs/inode.hpp"
#include "aex/printk.hpp"
#include "aex/types.hpp"

#include "chain.hpp"
#include "controlblock.hpp"

namespace AEX::FS {
    class FATINode : public INode {
        public:
        FATINode() {
            hard_links = 1;
            mode       = 0x0777;
        }

        protected:
        Chain m_chain;
        Mutex m_mutex;

        bool m_filled;

        Chain& chain() {
            return m_chain;
        }

        void refresh() {
            size = m_chain.count() * control_block->block_size;
        }

        friend class FATControlBlock;
    };

    class FATFileINode : public FATINode {
        public:
        FATFileINode() : FATINode() {
            type = FT_REGULAR;
        }

        error_t readBlocks(void* buffer, uint64_t start, uint16_t count);

        private:
        void fill();

        friend class FATDirectoryINode;
    };

    class FATDirectoryINode : public FATINode {
        public:
        FATDirectoryINode() : FATINode() {
            type = FT_DIRECTORY;
        }

        optional<dir_entry> readDir(dir_context* ctx);

        private:
        struct inode_assoc {
            char  filename[12];
            ino_t id;
        };

        Mem::Vector<inode_assoc> m_assocs;

        int readLFN(fat_dirent_lfn& lfn, char* buffer, int remaining);

        optional<ino_t> getAssoc(const char* filename);
        void            pushAssoc(const char* filename, ino_t id);
        ino_t           createAssoc(fat_dirent dirent);
    };
}