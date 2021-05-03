#include "aex/dev/blockdevice.hpp"

using namespace AEX::Dev;

namespace AEX::FS {
    class Partition : public BlockDevice {
        public:
        Partition(const char* name, BlockDevice* device, sct_t start, sctcnt_t count)
            : BlockDevice(name, device->sectorSize(), device->sectorCount(), device->maxCombo()) {
            m_base  = device;
            m_start = start;
            m_count = count;
        }

        error_t initBlock() {
            return m_base->initExt();
        }

        sctcnt_t readBlock(void* buffer, sct_t sector, sctcnt_t sector_count) {
            return m_base->readBlock(buffer, m_start + sector, sector_count);
        }

        sctcnt_t writeBlock(const void* buffer, sct_t sector, sctcnt_t sector_count) {
            return m_base->writeBlock(buffer, m_start + sector, sector_count);
        }

        error_t releaseBlock() {
            return m_base->releaseExt();
        }

        private:
        BlockDevice* m_base;
        sct_t        m_start;
        sctcnt_t     m_count;
    };
}