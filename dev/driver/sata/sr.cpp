#include "aex/dev/blockdevice.hpp"
#include "aex/dev/tree/driver.hpp"
#include "aex/dev/tree/tree.hpp"
#include "aex/endian.hpp"

#include "dev/driver/sata/satadevice.hpp"

namespace AEX::Dev::SATA {
    class SRBlock : public BlockDevice {
        public:
        SRBlock(SATADevice* device)
            : BlockDevice(device->name, SECTOR_SIZE, device->sector_count, device->max_page_burst) {
            _device = device;
        }

        private:
        static constexpr auto SECTOR_SIZE = 2048;

        SATADevice* _device;

        int64_t readBlock(uint8_t* buffer, uint64_t sector, uint32_t sector_count) {
            readWrite(buffer, sector, sector_count, false);
            return sector_count * SECTOR_SIZE;
        }
        int64_t writeBlock(uint8_t* buffer, uint64_t sector, uint32_t sector_count) {
            readWrite(buffer, sector, sector_count, true);
            return sector_count * SECTOR_SIZE;
        }

        void readWrite(uint8_t* buffer, uint32_t sector, uint32_t sector_count, bool write) {
            uint8_t packet[12] = {write ? AHCI::scsi_command::WRITE_12
                                        : AHCI::scsi_command::READ_12};

            *((uint32_t*) &(packet[2])) = bswap(sector);
            *((uint32_t*) &(packet[6])) = bswap(sector_count);

            _device->scsiPacket(packet, buffer, sector_count * SECTOR_SIZE);
        }
    };

    class SRDriver : public Tree::Driver {
        public:
        SRDriver() : Driver("sr") {}

        bool check(Tree::Device* _device) {
            auto device = (SATADevice*) _device;
            return device->type == type_t::SATAPI;
        }

        void bind(Tree::Device* _device) {
            auto device = (SATADevice*) _device;

            auto block = new SRBlock(device);
            if (!block->registerDevice()) {
                printk("sr: %s: Failed to register device\n", device->name);
                delete block;
            }
        }

        private:
        static constexpr auto SECTOR_SIZE = 2048;
    };

    void sr_init() {
        register_driver("sata", new SRDriver());
    }
}