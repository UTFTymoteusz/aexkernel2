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
            mode = 0x0777;
        }

        protected:
        INode_SP m_parent;
        Chain    m_chain;
        bool     m_filled;

        Chain& chain() {
            return m_chain;
        }

        void refresh() {
            size = m_chain.count() * controlblock->block_size;
        }

        friend class FATDirectory;
        friend class FATControlBlock;
    };

    class FATFile : public FATINode {
        public:
        FATFile() : FATINode() {
            type       = FT_REGULAR;
            hard_links = 1;
        }

        error_t read(void* buffer, blk_t start, blkcnt_t count);
        error_t write(const void* buffer, blk_t start, blkcnt_t count);
        error_t truncate(size_t newsize, bool cache);
        error_t purge();

        private:
        void fill();

        friend class FATDirectory;
    };

    class FATDirectory : public FATINode {
        public:
        FATDirectory() : FATINode() {
            type       = FT_DIRECTORY;
            hard_links = 2;
        }

        optional<INode_SP> creat(const char* filename, mode_t mode, fs_type_t type);
        optional<dirent>   readdir(dir_context* ctx);
        void               resize(INode* inode, cluster_t first, uint32_t size);
        error_t            link(const char* filename, INode_SP inode);
        error_t            unlink(const char* filename);
        error_t            purge();

        private:
        struct inode_assoc {
            char  filename[12];
            ino_t id;
        };

        Mem::Vector<inode_assoc> m_assocs;

        int  readLFN(fat_dirent_lfn& lfn, char* buffer, int remaining);
        void writeLFN(fat_dirent_lfn& lfn, const char* buffer, int remaining);

        fat_dirent read(int pos);
        void       write(int pos, fat_dirent& ent);
        void       write(int pos, fat_dirent& ent, const char* filename);

        optional<int> space(int count);
        error_t       expand();
        void          prepare(cluster_t cluster, cluster_t parent);
        void          prepare(cluster_t cluster);

        optional<ino_t>       getAssoc(const char* filename);
        optional<inode_assoc> getAssoc(ino_t id);
        void                  pushAssoc(const char* filename, ino_t id);
        void                  eraseAssoc(const char* filename);
        ino_t                 createAssoc(fat_dirent dirent);
    };
}