#pragma once

#include "aex/dev/bus.hpp"
#include "aex/dev/device.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"

#include <stdint.h>

namespace AEX::Dev {
    extern Mem::SmartArray<Bus>       buses;
    extern Mem::SmartArray<Interface> interfaces;

    /**
     * Tries to register a device in the specified bus.
     * @param bus_name The bus name.
     * @param device   The device.
     * @return True if succeeded.
     */
    bool register_device(const char* bus_name, Device* device);

    /**
     * Tries to register a driver in the specified bus.
     * @param bus_name The bus name.
     * @param driver   The driver.
     * @return True if succeeded.
     */
    bool register_driver(const char* bus_name, Driver* driver);

    /**
     * Gets a bus by it's name.
     * @param bus_name The bus name.
     * @return A smart pointer that points to the bus. Will be null on failure.
     */
    Mem::SmartPointer<Bus> getBus(const char* bus_name);

    /**
     * Checks if a bus exists.
     * @param bus_name The bus name.
     * @return True if the bus exists.
     */
    bool bus_exists(const char* bus_name);
}