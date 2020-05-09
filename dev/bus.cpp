#include "aex/dev/bus.hpp"

#include "aex/dev/tree.hpp"
#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
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
        auto scopeLock = ScopeSpinlock(_lock);

        children.addRef(device);

        printk("dev: %s: Registered device '%s'\n", this->name, device->name);

        for (auto iterator = _drivers.getIterator(); auto driver = iterator.next();)
            bindDriverToDevice(driver, device);
    }

    void Bus::registerDriver(Driver* driver) {
        auto scopeLock = ScopeSpinlock(_lock);

        _drivers.addRef(driver);

        printk("dev: %s: Registered driver '%s'\n", this->name, driver->name);

        for (auto iterator = children.getIterator(); auto device = iterator.next();) {
            printk("mmm?\n");

            bindDriverToDevice(driver, device);
        }
    }
}