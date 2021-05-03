#pragma once

#include "aex/dev/blockhandle.hpp"
#include "aex/dev/device.hpp"
#include "aex/dev/types.hpp"
#include "aex/errno.hpp"
#include "aex/mem.hpp"
#include "aex/optional.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class API BlockDevice : public Device {
        public:
        BlockDevice(const char* name, sctsize_t sector_size, sctcnt_t sector_count,
                    sctcnt_t max_sectors_at_once);

        virtual ~BlockDevice();

        virtual error_t  initBlock();
        virtual sctcnt_t readBlock(void* buffer, sct_t sector, sctcnt_t sector_count)        = 0;
        virtual sctcnt_t writeBlock(const void* buffer, sct_t sector, sctcnt_t sector_count) = 0;
        virtual error_t  releaseBlock();

        error_t initExt();
        error_t releaseExt();

        sctsize_t sectorSize() {
            return m_sector_size;
        }

        sctcnt_t sectorCount() {
            return m_sector_count;
        }

        sctcnt_t maxCombo() {
            return m_max_combo;
        }

        private:
        sctsize_t m_sector_size  = 512;
        sctcnt_t  m_sector_count = 0;
        sctcnt_t  m_max_combo    = 16;

        int32_t m_init_counter = 0;
        Mutex   m_init_lock;

        bool m_word_align = true;
    };

    typedef Mem::SmartPointer<BlockDevice> BlockDevice_SP;

    API optional<BlockHandle> open_block_handle(int id);
}