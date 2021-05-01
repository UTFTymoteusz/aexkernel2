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
#include "types.hpp"

namespace AEX::FS {
    class FATControlBlock : public ControlBlock {
        public:
        FATControlBlock(Dev::BlockHandle& handle, fat_info info) : block_handle(handle) {
            block_size = info.cluster_size;

            m_type = info.type;

            m_fat_start = info.fat_start;
            m_fat_size  = info.fat_size;
            m_fat_count = info.fat_count;

            m_data_offset   = info.data_offset;
            m_cluster_size  = info.cluster_size;
            m_cluster_count = info.cluster_count;

            m_root_cluster = info.root_first_cluster;
            root_inode_id  = nextINodeID();
        }

        Dev::BlockHandle block_handle;

        optional<INode_SP> getINode(INode_SP dir, dir_entry dirent, ino_t id);

        private:
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

        friend class FATDirectoryINode;
        friend class FATFileINode;
    };
}