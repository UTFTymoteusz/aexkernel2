#pragma once

#include "aex/dev/tree/device.hpp"

namespace AEX::Dev::PCI {
    class PCIDevice : public Tree::Device {
      public:
        uint16_t p_class, subclass, prog_if;
        uint16_t device_id, vendor_id;

        uint8_t bus, device, function;

        PCIDevice(const char* name) : Device(name) {}

      private:
    };

    void set_busmaster(Tree::Device* device, bool on);
}