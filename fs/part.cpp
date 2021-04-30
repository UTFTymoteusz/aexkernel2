#include "aex/fs/part.hpp"

#include "aex/dev/blockdevice.hpp"
#include "aex/dev/blockhandle.hpp"
#include "aex/errno.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/utility.hpp"

#include "fs/part.hpp"
#include "fs/part/msdos.hpp"

namespace AEX::FS {
    class Partition : public Dev::BlockDevice {
        public:
        Partition(const char* name, Dev::BlockDevice* device, uint64_t start, uint64_t count)
            : BlockDevice(name, device->sectorSize(), device->sectorCount(), device->maxCombo()) {
            m_base  = device;
            m_start = start;
            m_count = count;
        }

        error_t initBlock() {
            return m_base->initExt();
        }

        int64_t readBlock(void* buffer, uint64_t sector, uint32_t sector_count) {
            return m_base->readBlock(buffer, m_start + sector, sector_count);
        }

        int64_t writeBlock(const void* buffer, uint64_t sector, uint32_t sector_count) {
            return m_base->writeBlock(buffer, m_start + sector, sector_count);
        }

        error_t releaseBlock() {
            return m_base->releaseExt();
        }

        private:
        Dev::BlockDevice* m_base;
        uint64_t          m_start;
        uint64_t          m_count;
    };

    error_t read_partitions(Dev::BlockHandle handle, int limit) {
        partition     partitions[limit];
        optional<int> part_try;

        if ((part_try = read_msdos(handle, partitions, limit))) {
            int count = part_try.value;

            for (int i = 0; i < count; i++) {
                char name[32];
                snprintf(name, sizeof(name), "%s%i", handle.getDev()->name, i + 1);

                auto partition = new Partition(name, handle.getDev().get(), partitions[i].start,
                                               partitions[i].count);
                if (!partition->registerDevice()) {
                    printk("part: %s: Failed to register device\n", name);
                    delete partition;
                }
            }
        }
        else {
            return ENOTDIR;
        }

        return ENONE;
    }
}