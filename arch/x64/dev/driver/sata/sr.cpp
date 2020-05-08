#include "aex/dev/driver.hpp"
#include "aex/dev/tree.hpp"

#include "dev/driver/sata.hpp"

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
        }
    };

    void sr_init() {
        register_driver("sata", new SRDriver());
    }
}