#include "dev/dev.hpp"

#include "aex/dev.hpp"
#include "aex/dev/device.hpp"
#include "aex/dev/tree.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"

namespace AEX::Dev {
    namespace Tree {
        extern void init();
    }

    namespace TTY {
        void create_tty_devs();
    }

    Mem::SmartArray<Device> devices;

    extern void mainbus_init();
    extern void arch_drivers_init();

    void init() {
        printk(PRINTK_INIT "dev: Initializing\n");

        Tree::init();
        mainbus_init();
        arch_drivers_init();
        TTY::create_tty_devs();

        printk(PRINTK_OK "dev: Initialized\n");
    }
}