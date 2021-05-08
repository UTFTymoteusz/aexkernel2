#include "aex/utility.hpp"

#include "../inode.hpp"

namespace AEX::FS {
    error_t FATFileINode::read(void* buffer, blk_t start, blkcnt_t count) {
        if (!m_filled && m_chain.count() > 0)
            fill();

        AEX_ASSERT(start < m_chain.count());
        AEX_ASSERT(start + count <= m_chain.count());

        auto fat_block = (FATControlBlock*) control_block;
        for (uint64_t i = start; i < start + count; i++)
            fat_block->block_handle.read(buffer, fat_block->getOffset(m_chain.at(i), 0),
                                         block_size);

        return ENONE;
    }

    error_t FATFileINode::write(const void* buffer, blk_t start, blkcnt_t count) {
        if (!m_filled && m_chain.count() > 0)
            fill();

        AEX_ASSERT(start < m_chain.count());
        AEX_ASSERT(start + count <= m_chain.count());

        auto fat_block = (FATControlBlock*) control_block;
        for (uint64_t i = start; i < start + count; i++)
            fat_block->block_handle.write(buffer, fat_block->getOffset(m_chain.at(i), 0),
                                          block_size);

        return ENONE;
    }

    error_t FATFileINode::truncate(size_t newsize, bool cache) {
        if (newsize > 0xFFFFFFFF)
            return ENOTSUP;

        if (!m_filled && m_chain.count() > 0)
            fill();

        auto fat_block = (FATControlBlock*) control_block;
        SCOPE(fat_block->m_mutex);

        blkcnt_t count = (newsize - 1) / fat_block->m_cluster_size + 1;
        if (newsize == 0)
            count = 0;

        size = newsize;

        if (m_chain.count() != (size_t) count) {
            printk("truncate %i to %i\n", m_chain.count(), count);

            while (m_chain.count() < (size_t) count) {
                cluster_t found = fat_block->m_table->find();
                if (found == 0xFFFFFFFF)
                    return ENOSPC;

                fat_block->m_table->link(m_chain.last(), found);
                m_chain.push(found);
            }

            while (m_chain.count() > (size_t) count) {
                fat_block->m_table->link(m_chain.erase(), 0x00000000);
            }

            fat_block->m_table->link(m_chain.last(), 0x0FFFFFF8);

            if (!cache)
                fat_block->m_table->flush();
        }

        if (!cache) {
            auto dir = (FATDirectoryINode*) m_parent.get();
            using(dir->mutex) {
                dir->resize(this, newsize);
            }
        }

        return ENONE;
    }

    void FATFileINode::fill() {
        cluster_t first     = m_chain.at(0);
        auto      fat_block = (FATControlBlock*) control_block;
        fat_block->fillChain(first, m_chain);

        m_filled    = true;
        block_count = m_chain.count();
    }
}