#include "dev/dev.hpp"

#include "aex/dev/tree.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/printk.hpp"

// clang-format off

#include "sys/cpu.hpp"
#include "aex/dev/name.hpp"

// clang-format on

namespace AEX::Dev {
    extern void register_base_drivers();

    extern void mainbus_init();
    extern void arch_drivers_init();

    void init() {
        printk(PRINTK_INIT "dev: Initializing\n");

        buses = Mem::SmartArray<Bus>();

        mainbus_init();
        register_base_drivers();
        arch_drivers_init();

        printk(PRINTK_OK "dev: Initialized\n");

        char buffer[32];

        for (int i = 0; i < 70; i++) {
            name_letter_increment(buffer, "sd%");
            printk("%s;  ", buffer);
        }

        Sys::CPU::halt();
    }
}