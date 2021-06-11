#include "controlblock.hpp"

#include "aex/byte.hpp"

#include "inode.hpp"
#include "table/table32.hpp"

namespace AEX::FS {
    FATControlBlock::FATControlBlock(Dev::BlockHandle& handle, fat_info info)
        : block_handle(handle) {
        block_size = info.cluster_size;

        m_type = info.type;

        m_fat_start = info.fat_start;
        m_fat_size  = info.fat_size;
        m_fat_count = info.fat_count;

        m_data_offset   = info.data_offset;
        m_cluster_size  = info.cluster_size;
        m_cluster_count = info.cluster_count;

        m_table = new Table32(handle, info.sector_size, info.fat_start, info.fat_count,
                              info.cluster_count);

        m_root_cluster = info.root_first_cluster;
        root_inode_id  = nextIno();
    }

    FATControlBlock::~FATControlBlock() {
        delete m_table;
    }

    optional<INode_SP> FATControlBlock::get(INode_SP prev, dirent, ino_t id) {
        SCOPE(m_mutex);

        auto cache_try = m_cache.get(id);
        if (cache_try) {
            ((FATINode*) cache_try.value.get())->m_parent = prev;
            return cache_try.value;
        }

        if (id == root_inode_id)
            return createRoot();

        return {};
    }

    void FATControlBlock::unlink(ino_t id) {
        SCOPE(m_mutex);

        auto cache_try = m_cache.get(id);
        if (!cache_try)
            BROKEN;

        auto inode = cache_try.value;

        if (!inode->opened && inode->hard_links == 0) {
            inode->purge();
            m_cache.erase(id);
        }
    }

    uint64_t FATControlBlock::getOffset(cluster_t cluster, uint16_t local_offset) {
        return m_data_offset + (cluster - 2) * m_cluster_size + local_offset;
    }

    void FATControlBlock::fillChain(cluster_t start, Chain& chain) {
        SCOPE(m_table->mutex);

        cluster_t cluster = start;
        chain.m_clusters.clear();

        while (true) {
            chain.m_clusters.push(cluster);

            cluster = m_table->next(cluster);
            if (cluster == 0xFFFFFFFF)
                break;
        }
    }

    INode_SP FATControlBlock::createRoot() {
        auto root = new FATDirectory();
        auto sptr = INode_SP(root);

        root->controlblock = this;

        fillChain(m_root_cluster, root->chain());
        root->refresh();

        m_cache.set(root_inode_id, sptr);
        return sptr;
    }
}