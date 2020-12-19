#include "aex/dev/blockdevice.hpp"

#include "aex/assert.hpp"
#include "aex/dev.hpp"
#include "aex/dev/blockhandle.hpp"
#include "aex/dev/device.hpp"
#include "aex/errno.hpp"
#include "aex/math.hpp"
#include "aex/mem.hpp"
#include "aex/optional.hpp"
#include "aex/string.hpp"
#include "aex/types.hpp"

namespace AEX::Dev {
    BlockDevice::BlockDevice(const char* name, uint16_t sector_size, uint64_t sector_count,
                             uint16_t max_sectors_at_once)
        : Device(name, DEV_BLOCK) {
        m_sector_size  = sector_size;
        m_sector_count = sector_count;
        m_max_combo    = max_sectors_at_once;
    }

    BlockDevice::~BlockDevice() {
        //
    }

    error_t BlockDevice::initExt() {
        ScopeMutex lock(m_init_lock);

        if (m_init_counter == 0) {
            m_init_counter++;
            return initBlock();
        }

        m_init_counter++;
        return ENONE;
    }

    error_t BlockDevice::releaseExt() {
        ScopeMutex lock(m_init_lock);

        m_init_counter--;

        AEX_ASSERT(m_init_counter >= 0);
        if (m_init_counter == 0)
            return releaseBlock();

        return ENONE;
    }

    error_t BlockDevice::initBlock() {
        return ENONE;
    }

    int64_t BlockDevice::readBlock(void*, uint64_t, uint32_t) {
        return -1;
    }

    int64_t BlockDevice::writeBlock(const void*, uint64_t, uint32_t) {
        return -1;
    }

    error_t BlockDevice::releaseBlock() {
        return ENONE;
    }

    optional<BlockHandle> open_block_handle(int id) {
        auto device = devices.get(id);
        if (!device || device->type != DEV_BLOCK)
            return {};

        auto blk_device = (Dev::BlockDevice_SP) device;
        auto error      = blk_device->initExt();
        if (error)
            return error;

        return BlockHandle(blk_device);
    }
}