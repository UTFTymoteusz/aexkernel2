#pragma once

#include "aex/dev/tree/device.hpp"
#include "aex/dev/tree/driver.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::Tree {
    class Bus {
        public:
        Mem::SmartArray<Device> devices;
        Mem::SmartArray<Driver> drivers;

        char name[32];

        Bus(const char* name);

        /**
         * Registers a device in the bus.
         * @param device The device.
         */
        void registerDevice(Device* device);

        /**
         * Registers a driver in the bus.
         * @param driver The driver.
         */
        void registerDriver(Driver* driver);

        private:
        Spinlock _lock;

        static void bindDriverToDevice(Driver* driver, Device* device);
    };
}