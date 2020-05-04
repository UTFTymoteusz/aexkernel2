#include "aex/dev/tree.hpp"

#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
    RCPArray<Bus>       buses;
    RCPArray<Interface> interfaces;

    Spinlock lock;

    bool registerDevice(const char* bus_name, Device* device) {
        auto scopeLock = ScopeSpinlock(lock);

        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            bus->registerDevice(device);

            return true;
        }

        return false;
    }

    bool registerDriver(const char* bus_name, Driver* driver) {
        auto scopeLock = ScopeSpinlock(lock);

        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            bus->registerDriver(driver);

            return true;
        }

        return false;
    }
}