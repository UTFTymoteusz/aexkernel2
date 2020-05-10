#pragma once

#include "aex/dev/device.hpp"
#include "aex/dev/driver.hpp"
#include "aex/dev/interface.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/vector.hpp"
#include "aex/optional.hpp"

#include <stdint.h>

namespace AEX::Dev {
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