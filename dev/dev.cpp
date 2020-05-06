#include "dev/dev.hpp"

#include "aex/dev/tree.hpp"
#include "aex/printk.hpp"
#include "aex/rcparray.hpp"

namespace AEX::Dev {
    extern void register_base_interfaces();
    extern void register_base_drivers();

    extern void mainbus_init();
    extern void arch_drivers_init();

    void init() {
        printk(PRINTK_INIT "dev: Initializing\n");

        buses = RCPArray<Bus>();

        mainbus_init();
        arch_drivers_init();

        register_base_interfaces();
        register_base_drivers();

        printk(PRINTK_OK "dev: Initialized\n");
    }
}