#include "aex/utility.hpp"

#include "../inode.hpp"

namespace AEX::FS {
    error_t FATFile::read(void* buffer, blk_t start, blkcnt_t count) {
        if (!m_filled && m_chain.count() > 0)
            fill();

        ASSERT(start < m_chain.count());
        ASSERT(start + count <= m_chain.count());

        char* dst = (char*) buffer;

        auto fat_block = (FATControlBlock*) controlblock;
        for (uint64_t i = start; i < start + count; i++) {
            fat_block->block_handle.read(dst, fat_block->getOffset(m_chain.at(i), 0), block_size);
            dst += block_size;
        }

        return ENONE;
    }

    error_t FATFile::write(const void* buffer, blk_t start, blkcnt_t count) {
        if (!m_filled && m_chain.count() > 0)
            fill();

        ASSERT(start < m_chain.count());
        ASSERT(start + count <= m_chain.count());

        char* dst = (char*) buffer;

        auto fat_block = (FATControlBlock*) controlblock;
        for (uint64_t i = start; i < start + count; i++) {
            fat_block->block_handle.write(dst, fat_block->getOffset(m_chain.at(i), 0), block_size);
            dst += block_size;
        }

        return ENONE;
    }

    error_t FATFile::truncate(size_t newsize, bool cache) {
        if (newsize > 0xFFFFFFFF)
            return ENOTSUP;

        if (!m_filled && m_chain.count() > 0)
            fill();

        auto fat_block = (FATControlBlock*) controlblock;
        SCOPE(fat_block->m_mutex);

        blkcnt_t count = (newsize - 1) / fat_block->m_cluster_size + 1;
        if (newsize == 0)
            count = 0;

        size = newsize;

        if (m_chain.count() != (size_t) count) {
            SCOPE(fat_block->m_table->mutex);

            while (m_chain.count() < (size_t) count) {
                cluster_t found = fat_block->m_table->find();
                if (found == 0xFFFFFFFF)
                    return ENOSPC;

                if (m_chain.count() == 0) {
                    m_chain.push(found);
                    continue;
                }

                fat_block->m_table->link(m_chain.last(), found);
                m_chain.push(found);
            }

            while (m_chain.count() > (size_t) count) {
                fat_block->m_table->link(m_chain.erase(), 0x00000000);
            }

            if (m_chain.count() > 0)
                fat_block->m_table->link(m_chain.last(), 0x0FFFFFF8);

            if (!cache)
                fat_block->m_table->flush();
        }

        if (!cache) {
            auto dir = (FATDirectory*) m_parent.get();
            using(dir->mutex) {
                dir->resize(this, m_chain.count() > 0 ? m_chain.first() : 0, newsize);
            }
        }

        return ENONE;
    }

    error_t FATFile::purge() {
        printkd(PTKD_FS, "fat: %s: Purging file inode %i (%i clusters)\n", controlblock->path, id,
                m_chain.count());

        auto fat_block = (FATControlBlock*) controlblock;

        if (!m_filled && m_chain.count() > 0)
            fill();

        SCOPE(fat_block->m_table->mutex);

        for (size_t i = 0; i < m_chain.count(); i++)
            fat_block->m_table->link(m_chain.at(i), 0x00000000);

        fat_block->m_table->flush();

        return ENONE;
    }

    void FATFile::fill() {
        cluster_t first     = m_chain.at(0);
        auto      fat_block = (FATControlBlock*) controlblock;
        fat_block->fillChain(first, m_chain);

        m_filled    = true;
        block_count = m_chain.count();
    }
}