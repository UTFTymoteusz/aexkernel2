#include "dev/dev.hpp"

#include "aex/printk.hpp"
#include "aex/rcparray.hpp"

namespace AEX::Dev {
    extern void register_base_interfaces();
    extern void register_base_drivers();

    void init() {
        printk(PRINTK_INIT "dev: Initializing\n");

        register_base_interfaces();
        register_base_drivers();

        printk(PRINTK_OK "dev: Initialized\n");
    }
}