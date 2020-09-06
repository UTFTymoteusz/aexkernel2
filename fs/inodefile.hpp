#pragma once

#include "aex/fs/controlblock.hpp"
#include "aex/fs/file.hpp"
#include "aex/fs/inode.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::FS {
    class INodeFile : public File {
        public:
        INodeFile(INode_SP inode) {
            m_inode = inode;

            m_block_size   = m_inode->control_block->block_size;
            m_cache_buffer = new uint8_t[m_block_size];
        }

        ~INodeFile() {
            delete m_cache_buffer;
        }

        optional<uint32_t> read(void* buffer, uint32_t count) {
            uint32_t requested_count = count;

            count = min<uint32_t>(m_inode->size - m_pos, count);
            if (count == 0)
                return 0;

            if ((uint64_t) m_pos / m_block_size == m_cached_block) {
                uint16_t offset = m_pos & (m_block_size - 1);
                uint16_t len    = min<uint32_t>((uint32_t) m_block_size - offset, count);

                memcpy(buffer, m_cache_buffer + offset, len);

                count -= len;
                m_pos += len;

                buffer = (void*) ((uint8_t*) buffer + len);
            }

            if (count == 0)
                return requested_count;

            readBlocks(buffer, m_pos, count);

            m_pos += count;

            uint64_t last_block = (uint64_t) m_pos / m_block_size;
            if (last_block != m_cached_block)
                readBlocks(m_cache_buffer, last_block * m_block_size, m_block_size);

            m_cached_block = last_block;

            return requested_count;
        }

        optional<int64_t> seek(int64_t offset, seek_mode mode) {
            int64_t new_pos = 0;

            switch (mode) {
            case seek_mode::SEEK_SET:
                new_pos = offset;
                break;
            case seek_mode::SEEK_CURRENT:
                new_pos = m_pos + offset;
                break;
            case seek_mode::SEEK_END:
                new_pos = m_inode->size + offset;
                break;
            default:
                break;
            }

            if (new_pos < 0 || (uint64_t) new_pos > m_inode->size)
                return EINVAL;

            m_pos = new_pos;
            return new_pos;
        }

        optional<File_SP> dup() {
            auto dupd = new INodeFile(m_inode);

            dupd->m_pos          = m_pos;
            dupd->m_cached_block = m_cached_block;

            memcpy(dupd->m_cache_buffer, m_cache_buffer, m_block_size);

            return File_SP(dupd);
        }

        private:
        int64_t m_pos = 0;

        uint16_t m_block_size = 512;

        uint64_t m_cached_block = 0xFFFFFFFFFFFFFFFF;
        uint8_t* m_cache_buffer = nullptr;

        Mem::SmartPointer<INode> m_inode;

        void readBlocks(void* buffer, uint64_t start, uint32_t len) {
            bool combo = false;

            uint64_t combo_start = 0;
            uint32_t combo_count = 0;

            uint8_t m_overflow_buffer[m_block_size];

            while (len > 0) {
                uint32_t offset = start - int_floor<uint64_t>(start, m_block_size);
                uint32_t llen   = min(m_block_size - offset, len);

                if (!isPerfectFit(start, llen)) {
                    if (combo) {
                        m_inode->readBlocks(buffer, combo_start, combo_count);
                        buffer = (void*) ((uint8_t*) buffer + combo_count * m_block_size);

                        combo = false;
                    }

                    m_inode->readBlocks(m_overflow_buffer, start / m_block_size, 1);
                    memcpy(buffer, m_overflow_buffer + offset, llen);

                    buffer = (void*) ((uint8_t*) buffer + llen);
                }
                else {
                    if (!combo) {
                        combo = true;

                        combo_start = start / m_block_size;
                        combo_count = 0;
                    }

                    combo_count++;
                }

                start += llen;
                len -= llen;
            }

            if (combo)
                m_inode->readBlocks(buffer, combo_start, combo_count);
        }

        bool isPerfectFit(uint64_t start, uint32_t len) {
            if (start % m_block_size != 0)
                return false;

            if (len % m_block_size != 0)
                return false;

            return true;
        }
    };
}