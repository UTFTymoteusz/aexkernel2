#include "../inode.hpp"

namespace AEX::FS {
    error_t FATFileINode::readBlocks(void* buffer, uint64_t start, uint16_t count) {
        m_mutex.acquire();

        if (!m_filled && m_chain.count() > 0)
            fill();

        m_mutex.release();

        auto fat_block = (FATControlBlock*) control_block;
        for (uint64_t i = start; i < start + count; i++)
            fat_block->block_handle.read(buffer, fat_block->getOffset(m_chain.at(i), 0),
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