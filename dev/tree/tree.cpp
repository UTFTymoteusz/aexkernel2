#include "aex/dev/tree/tree.hpp"

#include "aex/mem/smartarray.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev::Tree {
    Mem::SmartArray<Bus> buses;

    Spinlock lock;

    bool register_device(const char* bus_name, Device* device) {
        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            bus->registerDevice(device);

            return true;
        }

        return false;
    }

    bool register_driver(const char* bus_name, Driver* driver) {
        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            bus->registerDriver(driver);

            return true;
        }

        return false;
    }

    Mem::SmartPointer<Bus> getBus(const char* bus_name) {
        int index = -1;

        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            index = iterator.index();
            break;
        }

        if (index == -1)
            return buses.get(-1);

        return buses.get(index);
    }

    bool bus_exists(const char* bus_name) {
        for (auto iterator = buses.getIterator(); auto bus = iterator.next();) {
            if (strcmp(bus->name, bus_name) != 0)
                continue;

            return true;
        }

        return false;
    }
}