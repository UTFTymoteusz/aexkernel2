#include "aex/dev/device.hpp"
#include "aex/dev/driver.hpp"
#include "aex/dev/tree.hpp"

namespace AEX::Dev {
    class SATA : public Driver {
      public:
        SATA() : Driver("sata") {}
        ~SATA() {}

        bool check(Device* device) {
            printk("sata: Check called\n");
            return false;
        }

      private:
    };

    void register_sata() {
        auto sata = new SATA();

        registerDriver("pci", sata);
    }
}