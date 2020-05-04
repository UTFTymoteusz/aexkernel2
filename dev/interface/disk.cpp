#include "aex/dev/interface.hpp"
#include "aex/printk.hpp"

namespace AEX::Dev {
    class Disk : public Interface {
      public:
        Disk() : Interface("disk") {}
        ~Disk() {}

        bool bind(Device* device) {
            printk("Disk: bind called\n");
            return false;
        }
    };

    void register_disk() {
        auto disk = new Disk();
    }
}