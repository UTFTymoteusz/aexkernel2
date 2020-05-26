#include "aex/dev/block.hpp"

#include "aex/dev/dev.hpp"
#include "aex/dev/device.hpp"
#include "aex/kpanic.hpp"
#include "aex/math.hpp"
#include "aex/string.hpp"

#include <stdint.h>

namespace AEX::Dev {
    Block::Block(const char* name, uint16_t sector_size, uint64_t sector_count,
                 uint16_t max_sectors_at_once)
        : Device(name, type_t::BLOCK) {
        _overflow_buffer = new uint8_t[sector_size];

        _sector_size         = sector_size;
        _sector_count        = sector_count;
        _max_sectors_at_once = max_sectors_at_once;

        printk(PRINTK_OK "Registered block device '%s'\n", name);
    }

    Block::~Block() {
        delete _overflow_buffer;
    }

    int Block::init() {
        return initBlock();
    }

    int64_t Block::read(uint8_t* buffer, uint64_t start, uint32_t len) {
        if (!isAligned(buffer) && word_align)
            kpanic("Implement misaligned block reads pls\n");

        bool combo = false;

        uint64_t combo_start = 0;
        uint32_t combo_count = 0;

        while (len > 0) {
            uint32_t offset = start - (start / _sector_size) * _sector_size;
            uint32_t llen   = min(_sector_size - offset, len);

            if (combo && combo_count == _max_sectors_at_once) {
                readBlock(buffer, combo_start, combo_count);
                buffer += combo_count * _sector_size;

                combo = false;
            }

            if (!isPerfectFit(start, llen)) {
                if (combo) {
                    readBlock(buffer, combo_start, combo_count);
                    buffer += combo_count * _sector_size;

                    combo = false;
                }

                readBlock(_overflow_buffer, start / _sector_size, 1);
                memcpy(buffer, _overflow_buffer + offset, llen);

                buffer += llen;
            }
            else {
                if (!combo) {
                    combo = true;

                    combo_start = start / _sector_size;
                    combo_count = 0;
                }

                combo_count++;
            }

            start += llen;
            len -= llen;
        }

        if (combo) {
            readBlock(buffer, combo_start, combo_count);
            buffer += combo_count * _sector_size;

            combo = false;
        }

        return 0;
    }
    int64_t Block::write(uint8_t*, uint64_t, uint32_t) {
        kpanic("Block::write is unimplemented, I'm too lazy atm\n");
    }

    void Block::release() {
        releaseBlock();
    }

    int Block::initBlock() {
        return 0;
    }

    int64_t Block::readBlock(uint8_t*, uint64_t, uint32_t) {
        return -1;
    }

    int64_t Block::writeBlock(uint8_t*, uint64_t, uint32_t) {
        return -1;
    }

    void Block::releaseBlock() {
        return;
    }

    bool Block::isAligned(uint8_t* addr) {
        return ((size_t) addr & 0x01) == 0;
    }

    bool Block::isPerfectFit(uint64_t start, uint32_t len) {
        if (start % _sector_size != 0)
            return false;

        if (len % _sector_size != 0)
            return false;

        return true;
    }
}