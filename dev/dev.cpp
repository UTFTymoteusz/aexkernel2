#include "dev/dev.hpp"

#include "aex/dev/device.hpp"
#include "aex/dev/tree/tree.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/printk.hpp"

namespace AEX::Dev {
    namespace Tree {
        extern void register_base_drivers();
    }

    Mem::SmartArray<Device> devices;

    extern void mainbus_init();
    extern void arch_drivers_init();

    void init() {
        printk(PRINTK_INIT "dev: Initializing\n");

        Tree::buses = Mem::SmartArray<Tree::Bus>();

        mainbus_init();
        Tree::register_base_drivers();
        arch_drivers_init();

        printk(PRINTK_OK "dev: Initialized\n");
    }
}