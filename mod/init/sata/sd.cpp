#include "aex/dev.hpp"
#include "aex/dev/tree.hpp"
#include "aex/fs/part.hpp"
#include "aex/printk.hpp"

#include "satadevice.hpp"

using namespace AEX::Dev;

namespace AEX::Sys::SATA {
    class SDBlock : public BlockDevice {
        public:
        SDBlock(SATADevice* device)
            : BlockDevice(device->name, SECTOR_SIZE, device->sector_count, device->max_page_burst) {
            m_device = device;
        }

        sctcnt_t readBlock(void* buffer, sct_t sector, sctcnt_t sector_count) {
            m_device->readWrite(buffer, sector, sector_count, false);
            return sector_count * SECTOR_SIZE;
        }

        sctcnt_t writeBlock(const void* buffer, sct_t sector, sctcnt_t sector_count) {
            m_device->readWrite((void*) buffer, sector, sector_count, true);
            return sector_count * SECTOR_SIZE;
        }

        private:
        static constexpr auto SECTOR_SIZE = 512;

        SATADevice* m_device;
    };

    class SDDriver : public Tree::Driver {
        public:
        SDDriver() : Driver("sd") {}

        bool check(Tree::Device* m_device) {
            auto device = (SATADevice*) m_device;
            return device->type == SATA_ATA;
        }

        void bind(Tree::Device* m_device) {
            auto device = (SATADevice*) m_device;
            auto block  = new SDBlock(device);
            if (!block->registerDevice()) {
                printk("sd: %s: Failed to register device\n", device->name);
                delete block;
            }

            FS::read_partitions(Dev::open_block_handle(block->id));
        }
    };

    void sd_init() {
        register_driver("sata", new SDDriver());
    }
}