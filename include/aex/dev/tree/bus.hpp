#pragma once

#include "aex/dev/tree/device.hpp"
#include "aex/dev/tree/driver.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"

#include <stdint.h>

namespace AEX::Dev::Tree {
    class Bus : public Device {
      public:
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
        Mem::SmartArray<Driver> _drivers;

        Spinlock _lock;

        static void bindDriverToDevice(Driver* driver, Device* device);
    };
}