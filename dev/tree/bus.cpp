#include "aex/dev/tree/bus.hpp"

#include "aex/dev/tree.hpp"
#include "aex/optional.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev::Tree {
    Bus::Bus(const char* name) : Device(name) {
        buses.addRef(this);

        printk(PRINTK_OK "dev: Registered bus '%s'\n", this->name);
    }

    void Bus::bindDriverToDevice(Driver* driver, Device* device) {
        if (!driver->check(device))
            return;

        driver->bind(device);
    }

    void Bus::registerDevice(Device* device) {
        children.addRef(device);

        printk("dev: %s: Registered tree device '%s'\n", this->name, device->name);

        for (auto iterator = _drivers.getIterator(); auto driver = iterator.next();)
            bindDriverToDevice(driver, device);
    }

    void Bus::registerDriver(Driver* driver) {
        _drivers.addRef(driver);

        printk("dev: %s: Registered tree driver '%s'\n", this->name, driver->name);

        for (auto iterator = children.getIterator(); auto device = iterator.next();)
            bindDriverToDevice(driver, device);
    }
}