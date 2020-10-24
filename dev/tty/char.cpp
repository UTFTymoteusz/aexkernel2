#include "aex/dev/tty.hpp"

#include "dev/tty/cdev.hpp"

namespace AEX::Dev::TTY {
    void create_tty_dev(int index);

    void create_tty_devs() {
        for (int i = 0; i < TTY_AMOUNT; i++)
            create_tty_dev(i);
    }

    void create_tty_dev(int index) {
        char name[32];
        snprintf(name, sizeof(name), "tty%i", index);

        auto device = new TTYChar(index, name);
        if (!device->registerDevice())
            kpanic("Failed to create device for %s", name);
    }
}