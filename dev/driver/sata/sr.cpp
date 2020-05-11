#include "aex/byte.hpp"
#include "aex/dev/driver.hpp"
#include "aex/dev/tree.hpp"

#include "dev/driver/sata/satadevice.hpp"

namespace AEX::Dev::SATA {
    class SRDriver : public Driver {
      public:
        SRDriver() : Driver("sr") {}

        bool check(Device* _device) {
            auto device = (SATADevice*) _device;
            return device->type == type_t::SATAPI;
        }

        void bind(Device* _device) {
            auto device = (SATADevice*) _device;

            uint8_t buffer[2048];

            read_write(device, buffer, 0, 1, false);

            printk("aaa: %s\n", &(buffer[0x39]));
        }

        void read_write(Device* _device, uint8_t* buffer, uint32_t sector, uint32_t sector_count,
                        bool write) {
            auto device = (SATADevice*) _device;

            uint8_t packet[12] = {write ? AHCI::scsi_command::WRITE_12
                                        : AHCI::scsi_command::READ_12};

            *((uint32_t*) &(packet[2])) = uint32_bswap(sector);
            *((uint32_t*) &(packet[6])) = uint32_bswap(sector_count);

            device->scsiPacket(packet, buffer, sector_count * SECTOR_SIZE);
        }

      private:
        static constexpr auto SECTOR_SIZE = 2048;
    };

    void sr_init() {
        register_driver("sata", new SRDriver());
    }
}