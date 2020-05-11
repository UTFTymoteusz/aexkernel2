#include "aex/byte.hpp"
#include "aex/dev/block.hpp"
#include "aex/dev/driver.hpp"
#include "aex/dev/tree.hpp"

#include "dev/driver/sata/satadevice.hpp"

namespace AEX::Dev::SATA {
    class SRBlock : public Block {
      public:
        SRBlock(SATADevice* device)
            : Block(SECTOR_SIZE, device->sector_count, device->max_page_burst) {
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

            *((uint32_t*) &(packet[2])) = uint32_bswap(sector);
            *((uint32_t*) &(packet[6])) = uint32_bswap(sector_count);

            _device->scsiPacket(packet, buffer, sector_count * SECTOR_SIZE);
        }
    };

    class SRDriver : public Driver {
      public:
        SRDriver() : Driver("sr") {}

        bool check(Device* _device) {
            auto device = (SATADevice*) _device;
            return device->type == type_t::SATAPI;
        }

        void bind(Device* _device) {
            auto device = (SATADevice*) _device;

            uint8_t buffer[20];
        }

      private:
        static constexpr auto SECTOR_SIZE = 2048;
    };

    void sr_init() {
        register_driver("sata", new SRDriver());
    }
}