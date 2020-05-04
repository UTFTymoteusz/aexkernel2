#pragma once

#include "aex/dev/bus.hpp"
#include "aex/dev/device.hpp"
#include "aex/optional.hpp"
#include "aex/rcparray.hpp"
#include "aex/vector.hpp"

#include <stdint.h>

namespace AEX::Dev {
    extern RCPArray<Bus>       buses;
    extern RCPArray<Interface> interfaces;

    extern Spinlock lock;

    /**
     * Tries to register a device in the specified bus.
     * @param bus_name The bus name.
     * @param device   The device.
     * @return True if succeeded.
     */
    bool registerDevice(const char* bus_name, Device* device);

    /**
     * Tries to register a driver in the specified bus.
     * @param bus_name The bus name.
     * @param driver   The driver.
     * @return True if succeeded.
     */
    bool registerDriver(const char* bus_name, Driver* driver);
}