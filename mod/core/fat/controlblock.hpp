#pragma once

#include "aex/dev.hpp"
#include "aex/errno.hpp"
#include "aex/fs/controlblock.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"
#include "aex/fs/types.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"

#include "chain.hpp"
#include "table.hpp"
#include "types.hpp"

namespace AEX::FS {
    class FATControlBlock : public ControlBlock {
        public:
        Dev::BlockHandle block_handle;

        FATControlBlock(Dev::BlockHandle& handle, fat_info info);
        ~FATControlBlock();

        optional<INode_SP> get(INode_SP dir, dirent dirent, ino_t id);
        void               unlink(ino_t);

        private:
        Table* m_table;

        fat_type m_type;

        off_t    m_fat_start;
        size_t   m_fat_size;
        uint16_t m_fat_count;

        off_t     m_data_offset;
        blksize_t m_cluster_size;
        uint32_t  m_cluster_count;

        uint32_t m_root_cluster;

        uint64_t getOffset(uint32_t cluster, uint16_t local_offset);
        void     fillChain(uint32_t start, Chain& chain);
        INode_SP createRoot();

        friend class FATINode;
        friend class FATDirectory;
        friend class FATFile;
    };
}