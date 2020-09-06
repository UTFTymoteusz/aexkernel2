#pragma once

#include "aex/dev/device.hpp"
#include "aex/mem.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    class BlockDevice : public Device {
        public:
        BlockDevice(const char* name, uint16_t sector_size, uint64_t sector_count,
                    uint16_t max_sectors_at_once);

        virtual ~BlockDevice();

        int     init();
        int64_t read(void* buffer, uint64_t start, uint32_t len);
        int64_t write(const void* buffer, uint64_t start, uint32_t len);
        void    release();

        private:
        uint8_t* m_overflow_buffer;

        uint16_t m_sector_size         = 512;
        uint64_t m_sector_count        = 0;
        uint16_t m_max_sectors_at_once = 16;

        bool word_align = true;

        virtual int     initBlock();
        virtual int64_t readBlock(void* buffer, uint64_t sector, uint32_t sector_count)        = 0;
        virtual int64_t writeBlock(const void* buffer, uint64_t sector, uint32_t sector_count) = 0;
        virtual void    releaseBlock();

        bool isAligned(void* addr);
        bool isPerfectFit(uint64_t start, uint32_t len);
    };

    typedef Mem::SmartPointer<BlockDevice> BlockDevice_SP;

    BlockDevice_SP get_block_device(int id);
}