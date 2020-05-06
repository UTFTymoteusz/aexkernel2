#pragma once

#include "aex/dev/device.hpp"

namespace AEX::Dev::PCI {
    class PCIDevice : public Device {
      public:
        struct bar {
            enum type_t : uint8_t {
                IO     = 0,
                MEMORY = 1,
            };

            type_t type;

            size_t start;
            size_t len;
        };

        uint16_t p_class, subclass, prog_if;
        uint16_t device_id, vendor_id;

        uint8_t bus, device, function;

        PCIDevice(const char* name) : Device(name) {}

      private:
    };

    void set_busmaster(Device* device, bool on);
}