#pragma once

#include "aex/dev/tree/bus.hpp"
#include "aex/dev/tree/device.hpp"
#include "aex/dev/tree/driver.hpp"
#include "aex/mem.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::Tree {
    API extern Mem::SmartArray<Bus> buses;
    API extern Tree::Device*        root_device;

    /**
     * Tries to register a device in the specified bus.
     * @param bus_name The bus name.
     * @param device   The device.
     * @returns True if succeeded.
     **/
    API bool register_device(const char* bus_name, Device* device);

    /**
     * Tries to register a driver in the specified bus.
     * @param bus_name The bus name.
     * @param driver   The driver.
     * @returns True if succeeded.
     **/
    API bool register_driver(const char* bus_name, Driver* driver);

    /**
     * Gets a bus by it's name.
     * @param bus_name The bus name.
     * @returns A smart pointer that points to the bus. Will be null on failure.
     **/
    API Mem::SmartPointer<Bus> get_bus(const char* bus_name);

    /**
     * Checks if a bus exists.
     * @param bus_name The bus name.
     * @returns True if the bus exists.
     **/
    API bool bus_exists(const char* bus_name);
}