#include "aex/dev/blockdevice.hpp"

#include "aex/dev.hpp"
#include "aex/dev/device.hpp"
#include "aex/kpanic.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/proc.hpp"
#include "aex/string.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    BlockDevice::BlockDevice(const char* name, uint16_t sector_size, uint64_t sector_count,
                             uint16_t max_sectors_at_once)
        : Device(name, DEV_BLOCK) {
        m_overflow_buffer = new uint8_t[sector_size];

        m_sector_size         = sector_size;
        m_sector_count        = sector_count;
        m_max_sectors_at_once = max_sectors_at_once;
    }

    BlockDevice::~BlockDevice() {
        delete m_overflow_buffer;
    }

    int BlockDevice::init() {
        return initBlock();
    }

    int64_t BlockDevice::read(void* buffer, uint64_t start, uint32_t len) {
        auto current_usage = &Proc::Thread::getCurrent()->getProcess()->usage;

        // this needs to be checked properly
        while (!isAligned(buffer) && word_align) {
            uint8_t misaligned_sector[m_sector_size];
            readBlock(misaligned_sector, start / m_sector_size, 1);

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

            if (combo && combo_count == m_max_sectors_at_once) {
                readBlock(buffer, combo_start, combo_count);

                current_usage->block_bytes_read += combo_count * m_sector_size;

                buffer = (void*) ((uint8_t*) buffer + combo_count * m_sector_size);
                combo  = false;
            }

            if (!isPerfectFit(start, llen)) {
                if (combo) {
                    readBlock(buffer, combo_start, combo_count);

                    buffer = (void*) ((uint8_t*) buffer + combo_count * m_sector_size);
                    combo  = false;
                }

                readBlock(m_overflow_buffer, start / m_sector_size, 1);
                memcpy(buffer, m_overflow_buffer + offset, llen);

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
            readBlock(buffer, combo_start, combo_count);
            current_usage->block_bytes_read += combo_count * m_sector_size;
        }

        return 0;
    }

    int64_t BlockDevice::write(const void*, uint64_t, uint32_t) {
        kpanic("Block::write is unimplemented, I'm too lazy atm\n");
    }

    void BlockDevice::release() {
        releaseBlock();
    }

    int BlockDevice::initBlock() {
        return 0;
    }

    int64_t BlockDevice::readBlock(void*, uint64_t, uint32_t) {
        return -1;
    }

    int64_t BlockDevice::writeBlock(const void*, uint64_t, uint32_t) {
        return -1;
    }

    void BlockDevice::releaseBlock() {
        return;
    }

    bool BlockDevice::isAligned(void* addr) {
        return ((size_t) addr & 0x01) == 0;
    }

    bool BlockDevice::isPerfectFit(uint64_t start, uint32_t len) {
        if (start % m_sector_size != 0)
            return false;

        if (len % m_sector_size != 0)
            return false;

        return true;
    }

    Dev::BlockDevice_SP get_block_device(int id) {
        auto device = devices.get(id);
        if (!device || device->type != DEV_BLOCK)
            return devices.get(-1);

        return device;
    }
}