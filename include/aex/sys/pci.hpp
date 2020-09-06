#pragma once

#include "aex/dev/tree/device.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::PCI {
    class PCIDevice : public Dev::Tree::Device {
        public:
        uint16_t p_class, subclass, prog_if;
        uint16_t device_id, vendor_id;

        uint8_t bus, device, function;

        uint8_t interrupt_pin;

        PCIDevice(const char* name, Dev::Tree::Device* parent) : Device(name, parent) {}

        /**
         * Gets the IRQ number of the device.
         * @returns IRQ number or 255 on failure.
         */
        int getIRQ();
    };

    void set_busmaster(Dev::Tree::Device* device, bool on);

    uint8_t  read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint16_t read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint32_t read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    void write_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t val);
    void write_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t val);
    void write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t val);
}