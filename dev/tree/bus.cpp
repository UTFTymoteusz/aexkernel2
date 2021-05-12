#include "aex/dev/tree/bus.hpp"

#include "aex/dev/tree.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev::Tree {
    Bus::Bus(const char* name) {
        strncpy(this->name, name, sizeof(this->name));

        buses.addRef(this);

        printk(OK "dev: Registered bus '%s'\n", this->name);
    }

    void Bus::bind(Driver* driver, Device* device) {
        if (!driver->check(device))
            return;

        driver->bind(device);
    }

    void Bus::registerDevice(Device* device) {
        devices.addRef(device);

        printk("dev: %s: Registered tree device '%s'\n", this->name, device->name);

        for (auto iterator = drivers.getIterator(); auto driver = iterator.next();)
            bind(driver, device);
    }

    void Bus::registerDriver(Driver* driver) {
        drivers.addRef(driver);

        printk("dev: %s: Registered tree driver '%s'\n", this->name, driver->name);

        for (auto iterator = devices.getIterator(); auto device = iterator.next();)
            bind(driver, device);
    }
}