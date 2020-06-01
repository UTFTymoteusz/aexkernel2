#pragma once

#include "aex/dev/tree/device.hpp"

#include <stdint.h>

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

    uint8_t  read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint16_t read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    void write_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t val);
    void write_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t val);
    void write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t val);
}