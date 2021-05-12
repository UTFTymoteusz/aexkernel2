#include "aex/dev/blockhandle.hpp"

#include "aex/assert.hpp"
#include "aex/dev/blockdevice.hpp"
#include "aex/proc.hpp"
#include "aex/types.hpp"

namespace AEX::Dev {
    BlockHandle::BlockHandle() {
        m_dev = BlockDevice_SP::null();
    }

    BlockHandle::BlockHandle(BlockDevice_SP blkdev) : m_dev(blkdev) {
        m_shared           = new shared();
        m_shared->m_buffer = new uint8_t[blkdev->sectorSize()];
        m_sector_size      = blkdev->sectorSize();
    }

    BlockHandle::BlockHandle(const BlockHandle& bh) : m_dev(bh.m_dev) {
        m_shared = bh.m_shared;
        m_shared->m_refs.increment();

        m_sector_size = bh.m_sector_size;
    }

    BlockHandle::~BlockHandle() {
        if (!m_shared)
            return;

        if (!m_shared->m_refs.decrement())
            return;

        if (m_dev.isValid())
            m_dev->releaseExt();

        delete m_shared->m_buffer;
        delete m_shared;

        m_shared = nullptr;
    }

    int64_t BlockHandle::read(void* buffer, uint64_t start, uint32_t len) {
        AEX_ASSERT(m_dev.isValid());
        AEX_ASSERT(m_shared);

        auto current_usage = &Proc::Thread::current()->getProcess()->usage;

        // this needs to be checked properly
        while (!isAligned(buffer)) {
            uint8_t misaligned_sector[m_sector_size];
            m_dev->readBlock(misaligned_sector, start / m_sector_size, 1);

            current_usage->block_bytes_read += m_sector_size;

            uint16_t misaligned_len = min<uint64_t>(start & (m_sector_size - 1), len);
            if (misaligned_len == 0) // If this is the case, we're aligned sector-wise
                misaligned_len = min<uint16_t>(len, m_sector_size);

            memcpy(buffer, misaligned_sector, misaligned_len);

            buffer = (void*) ((uint8_t*) buffer + misaligned_len);
            start += misaligned_len;
            len -= misaligned_len;

            if (len == 0)
                return 0;
        }

        bool combo = false;

        uint64_t combo_start = 0;
        uint32_t combo_count = 0;

        while (len > 0) {
            uint32_t offset = start - int_floor<uint64_t>(start, m_sector_size);
            uint32_t llen   = min(m_sector_size - offset, len);

            if (combo && combo_count == m_dev->maxCombo()) {
                m_dev->readBlock(buffer, combo_start, combo_count);

                current_usage->block_bytes_read += combo_count * m_sector_size;

                buffer = (void*) ((uint8_t*) buffer + combo_count * m_sector_size);
                combo  = false;
            }

            if (!isPerfect(start, llen)) {
                if (combo) {
                    m_dev->readBlock(buffer, combo_start, combo_count);

                    buffer = (void*) ((uint8_t*) buffer + combo_count * m_sector_size);
                    combo  = false;
                }

                m_shared->m_mutex.acquire();

                m_dev->readBlock(m_shared->m_buffer, start / m_sector_size, 1);
                memcpy(buffer, m_shared->m_buffer + offset, llen);

                m_shared->m_mutex.release();

                current_usage->block_bytes_read += m_sector_size;

                buffer = (void*) ((uint8_t*) buffer + llen);
            }
            else {
                if (!combo) {
                    combo = true;

                    combo_start = start / m_sector_size;
                    combo_count = 0;
                }

                combo_count++;
            }

            start += llen;
            len -= llen;
        }

        if (combo) {
            m_dev->readBlock(buffer, combo_start, combo_count);
            current_usage->block_bytes_read += combo_count * m_sector_size;
        }

        return 0;
    }

    int64_t BlockHandle::write(const void* buffer, uint64_t start, uint32_t len) {
        AEX_ASSERT(m_dev.isValid());
        AEX_ASSERT(m_shared);

        auto current_usage = &Proc::Thread::current()->getProcess()->usage;

        // this needs to be checked properly
        while (!isAligned(buffer)) {
            uint8_t misaligned_sector[m_sector_size];
            m_dev->readBlock(misaligned_sector, start / m_sector_size, 1);

            current_usage->block_bytes_read += m_sector_size;

            uint16_t misaligned_len = min<uint64_t>(start & (m_sector_size - 1), len);
            if (misaligned_len == 0) // If this is the case, we're aligned sector-wise
                misaligned_len = min<uint16_t>(len, m_sector_size);

            memcpy(misaligned_sector, buffer, misaligned_len);

            m_dev->writeBlock(misaligned_sector, start / m_sector_size, 1);

            buffer = (void*) ((uint8_t*) buffer + misaligned_len);
            start += misaligned_len;
            len -= misaligned_len;

            if (len == 0)
                return 0;
        }

        bool combo = false;

        uint64_t combo_start = 0;
        uint32_t combo_count = 0;

        while (len > 0) {
            uint32_t offset = start - int_floor<uint64_t>(start, m_sector_size);
            uint32_t llen   = min(m_sector_size - offset, len);

            if (combo && combo_count == m_dev->maxCombo()) {
                m_dev->writeBlock(buffer, combo_start, combo_count);

                current_usage->block_bytes_read += combo_count * m_sector_size;

                buffer = (void*) ((uint8_t*) buffer + combo_count * m_sector_size);
                combo  = false;
            }

            if (!isPerfect(start, llen)) {
                if (combo) {
                    m_dev->writeBlock(buffer, combo_start, combo_count);

                    buffer = (void*) ((uint8_t*) buffer + combo_count * m_sector_size);
                    combo  = false;
                }

                m_shared->m_mutex.acquire();

                m_dev->readBlock(m_shared->m_buffer, start / m_sector_size, 1);
                memcpy(m_shared->m_buffer + offset, buffer, llen);
                m_dev->writeBlock(m_shared->m_buffer, start / m_sector_size, 1);

                m_shared->m_mutex.release();

                current_usage->block_bytes_read += m_sector_size;

                buffer = (void*) ((uint8_t*) buffer + llen);
            }
            else {
                if (!combo) {
                    combo = true;

                    combo_start = start / m_sector_size;
                    combo_count = 0;
                }

                combo_count++;
            }

            start += llen;
            len -= llen;
        }

        if (combo) {
            m_dev->writeBlock(buffer, combo_start, combo_count);
            current_usage->block_bytes_read += combo_count * m_sector_size;
        }

        return 0;
    }

    Mem::SmartPointer<BlockDevice> BlockHandle::getDev() {
        return m_dev;
    }

    bool BlockHandle::isAligned(const void* addr) {
        return ((size_t) addr & 0x01) == 0;
    }

    bool BlockHandle::isPerfect(uint64_t start, uint32_t len) {
        if (start % m_sector_size != 0)
            return false;

        if (len % m_sector_size != 0)
            return false;

        return true;
    }
}