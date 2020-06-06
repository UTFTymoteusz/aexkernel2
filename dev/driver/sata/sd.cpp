#include "aex/dev/tree/driver.hpp"
#include "aex/dev/tree/tree.hpp"

#include "dev/driver/sata/satadevice.hpp"

namespace AEX::Dev::SATA {
    class SDDriver : public Tree::Driver {
        public:
        SDDriver() : Driver("sd") {}

        bool check(Tree::Device* _device) {
            auto device = (SATADevice*) _device;
            return device->type == type_t::SATA;
        }

        void bind(Tree::Device* _device) {
            auto device = (SATADevice*) _device;
        }
    };

    void sd_init() {
        register_driver("sata", new SDDriver());
    }
}