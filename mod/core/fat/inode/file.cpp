#include "aex/utility.hpp"

#include "../inode.hpp"

namespace AEX::FS {
    error_t FATFileINode::readBlocks(void* buffer, blk_t start, blkcnt_t count) {
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

    error_t FATFileINode::writeBlocks(const void* buffer, blk_t start, blkcnt_t count) {
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

    void FATFileINode::fill() {
        cluster_t first     = m_chain.at(0);
        auto      fat_block = (FATControlBlock*) control_block;
        fat_block->fillChain(first, m_chain);

        m_filled    = true;
        block_count = m_chain.count();
    }
}