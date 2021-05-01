#include "controlblock.hpp"

#include "aex/byte.hpp"

#include "inode.hpp"

namespace AEX::FS {
    optional<INode_SP> FATControlBlock::getINode(INode_SP dir, dirent dirent, ino_t id) {
        auto scope     = ScopeMutex(m_mutex);
        auto cache_try = m_cache.get(id);

        if (cache_try.has_value)
            return cache_try.value;

        if (id == root_inode_id)
            return createRoot();

        return {};
    }

    uint64_t FATControlBlock::getOffset(uint32_t cluster, uint16_t local_offset) {
        return m_data_offset + cluster * m_cluster_size + local_offset;
    }

    void FATControlBlock::fillChain(uint32_t start, Chain& chain) {
        uint32_t cluster = start;

        chain.m_clusters.clear();

        while (true) {
            little_endian<uint32_t> next;

            chain.m_clusters.push(cluster);
            block_handle.read(&next, m_fat_start + cluster * sizeof(uint32_t), sizeof(uint32_t));

            cluster = next & 0x0FFFFFFF;
            if (cluster >= 0x0FFFFFF7)
                break;
        }
    }

    INode_SP FATControlBlock::createRoot() {
        auto root = new FATDirectoryINode();
        auto sptr = INode_SP(root);

        root->control_block = this;

        fillChain(m_root_cluster, root->chain());
        root->refresh();

        m_cache.set(root_inode_id, sptr);
        return sptr;
    }
}