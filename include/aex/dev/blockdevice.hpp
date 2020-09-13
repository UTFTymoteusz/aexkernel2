#pragma once

#include "aex/dev/blockhandle.hpp"
#include "aex/dev/device.hpp"
#include "aex/errno.hpp"
#include "aex/mem.hpp"
#include "aex/optional.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class BlockDevice : public Device {
        public:
        BlockDevice(const char* name, uint16_t sector_size, uint64_t sector_count,
                    uint16_t max_sectors_at_once);

        virtual ~BlockDevice();

        error_t initExt();
        error_t releaseExt();

        virtual error_t initBlock();
        virtual int64_t readBlock(void* buffer, uint64_t sector, uint32_t sector_count)        = 0;
        virtual int64_t writeBlock(const void* buffer, uint64_t sector, uint32_t sector_count) = 0;
        virtual error_t releaseBlock();

        uint16_t sectorSize() {
            return m_sector_size;
        }

        uint16_t maxCombo() {
            return m_max_combo;
        }

        private:
        uint16_t m_sector_size  = 512;
        uint64_t m_sector_count = 0;
        uint16_t m_max_combo    = 16;

        int32_t m_init_counter = 0;
        Mutex   m_init_lock;

        bool m_word_align = true;
    };

    typedef Mem::SmartPointer<BlockDevice> BlockDevice_SP;

    optional<BlockHandle> open_block_handle(int id);
}