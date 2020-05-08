#include "aex/dev/driver.hpp"
#include "aex/dev/tree.hpp"

#include "dev/driver/sata/satadevice.hpp"

namespace AEX::Dev::SATA {
    class SDDriver : public Driver {
      public:
        SDDriver() : Driver("sd") {}

        bool check(Device* _device) {
            auto device = (SATADevice*) _device;
            return device->type == type_t::SATA;
        }

        void bind(Device* _device) {
            auto device = (SATADevice*) _device;
        }
    };

    void sd_init() {
        register_driver("sata", new SDDriver());
    }
}