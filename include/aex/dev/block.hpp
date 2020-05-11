#pragma once

#include "aex/dev/device.hpp"
#include "aex/printk.hpp"

#include <stddef.h>
#include <stdint.h>


namespace AEX::Dev {
    class Block : public Device {
      public:
        Block(uint16_t sector_size, uint64_t sector_count, uint16_t max_sectors_at_once);
        ~Block();

        int     init();
        int64_t read(uint8_t* buffer, uint64_t start, uint32_t len);
        int64_t write(uint8_t* buffer, uint64_t start, uint32_t len);
        void    release();

      private:
        uint8_t* _overflow_buffer;

        uint16_t _sector_size         = 512;
        uint64_t _sector_count        = 0;
        uint16_t _max_sectors_at_once = 16;

        bool word_align = true;

        virtual int     initBlock();
        virtual int64_t readBlock(uint8_t* buffer, uint64_t sector, uint32_t sector_count)  = 0;
        virtual int64_t writeBlock(uint8_t* buffer, uint64_t sector, uint32_t sector_count) = 0;
        virtual void    releaseBlock();

        bool isAligned(uint8_t* addr);
        bool isPerfectFit(uint64_t start, uint32_t len);
    };
}