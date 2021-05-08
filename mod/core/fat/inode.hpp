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
        INode_SP m_parent;
        Chain    m_chain;
        bool     m_filled;

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

        error_t read(void* buffer, blk_t start, blkcnt_t count);
        error_t write(const void* buffer, blk_t start, blkcnt_t count);
        error_t truncate(size_t newsize, bool cache);

        private:
        void fill();

        friend class FATDirectoryINode;
    };

    class FATDirectoryINode : public FATINode {
        public:
        FATDirectoryINode() : FATINode() {
            type = FT_DIRECTORY;
        }

        optional<dirent> readDir(dir_context* ctx);
        void             resize(INode* inode, uint32_t size);

        private:
        struct inode_assoc {
            char  filename[12];
            ino_t id;
        };

        Mem::Vector<inode_assoc> m_assocs;

        int readLFN(fat_dirent_lfn& lfn, char* buffer, int remaining);

        optional<ino_t>       getAssoc(const char* filename);
        optional<inode_assoc> getAssoc(ino_t id);
        void                  pushAssoc(const char* filename, ino_t id);
        ino_t                 createAssoc(fat_dirent dirent);
    };
}