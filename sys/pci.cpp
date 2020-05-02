#include "sys/pci.hpp"

#include "aex/dev/tree.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"

#include "sys/cpu.hpp"

#include <stdint.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

namespace AEX::Sys::PCI {
    Dev::Bus* dev_bus;

    Spinlock lock;

    uint16_t read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint8_t  read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    void scan_all_buses();
    void scan_device(uint16_t bus, uint8_t device);

    uint16_t get_vendor_id(uint16_t bus, uint8_t device, uint8_t function);
    uint16_t get_device_id(uint16_t bus, uint8_t device, uint8_t function);

    uint8_t get_class(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_subclass(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_prog_if(uint16_t bus, uint8_t device, uint8_t function);

    void init() {
        printk(PRINTK_INIT "pci: Initializing\n");

        dev_bus = new Dev::Bus("pci");
        scan_all_buses();

        printk(PRINTK_OK "pci: Initialized\n");
    }

    uint32_t read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
        uint32_t address = 0x00;

        address = (uint32_t)((uint32_t) bus << 16) | ((uint32_t) device << 11) |
                  ((uint32_t) function << 8) | (offset & 0xFC) | (1 << 31);

        auto scopeLock = ScopeSpinlock(lock);

        Sys::CPU::outportd(CONFIG_ADDRESS, address);

        return Sys::CPU::inportd(CONFIG_DATA);
    }

    uint16_t read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
        uint32_t address = 0x00;

        address = (uint32_t)((uint32_t) bus << 16) | ((uint32_t) device << 11) |
                  ((uint32_t) function << 8) | (offset & 0xFC) | (1 << 31);

        auto scopeLock = ScopeSpinlock(lock);

        Sys::CPU::outportd(CONFIG_ADDRESS, address);

        return (uint16_t)((Sys::CPU::inportd(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    }

    uint8_t read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
        uint32_t address = 0x00;

        address = (uint32_t)((uint32_t) bus << 16) | ((uint32_t) device << 11) |
                  ((uint32_t) function << 8) | (offset & 0xFC) | (1 << 31);

        auto scopeLock = ScopeSpinlock(lock);

        Sys::CPU::outportd(CONFIG_ADDRESS, address);

        return (uint16_t)((Sys::CPU::inportd(CONFIG_DATA) >> ((offset & 3) * 8)) & 0xFF);
    }

    void write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t val) {
        uint32_t address = 0x00;

        address = (uint32_t)((uint32_t) bus << 16) | ((uint32_t) device << 11) |
                  ((uint32_t) function << 8) | (offset & 0xFC) | (1 << 31);

        auto scopeLock = ScopeSpinlock(lock);

        Sys::CPU::outportd(CONFIG_ADDRESS, address);

        return Sys::CPU::outportd(CONFIG_DATA, val);
    }

    void scan_all_buses() {
        for (uint16_t bus = 0; bus < 256; bus++)
            for (uint8_t device = 0; device < 32; device++)
                scan_device(bus, device);
    }

    void fill_bars(uint8_t bus, uint8_t device, uint8_t function, Dev::Device* dev_device) {
        PMem::phys_addr bar0, bar1;

        PMem::phys_addr addr;
        size_t          len;

        uint32_t offset = 16;
        uint32_t mask   = 0;
        uint8_t  type;
        bool     io;

        for (int i = 0; i < 6; i++) {
            bar0 = read_dword(bus, device, function, offset);
            bar1 = read_dword(bus, device, function, offset + 4);
            type = (bar0 >> 1) & 0x03;
            io   = (bar0 & 0x01) > 0;

            if (bar0 == 0) {
                offset += 4;
                continue;
            }

            mask = io ? 0xFFFFFFFC : 0xFFFFFFF0;
            addr = bar0 & mask;

            if (type == 0x02)
                addr |= bar1 << 32;

            write_dword(bus, device, function, offset, 0xFFFFFFFF);
            len = ~(read_dword(bus, device, function, offset) & mask) + 1;

            if (type == 0x01)
                len &= 0xFFFF;
            if ((len & 0xFFFF0000) > 0)
                len &= 0xFFFF;

            if (io) {
                auto resource = Dev::Device::resource();

                resource.type  = Dev::Device::resource::type::IO;
                resource.value = addr;
                resource.end   = addr + len;

                dev_device->addResource(resource);
            }
            else {
                auto resource = Dev::Device::resource();

                resource.type  = Dev::Device::resource::type::MEMORY;
                resource.value = addr;
                resource.end   = addr + len;

                dev_device->addResource(resource);
            }

            write_dword(bus, device, function, offset, bar0);

            if (type == 0x02)
                offset += 4;

            offset += 4;
        }
    }

    void scan_device(uint16_t bus, uint8_t device) {
        if (get_vendor_id(bus, device, 0x00) == 0xFFFF)
            return;

        for (uint8_t function = 0; function < 8; function++) {
            if (get_vendor_id(bus, device, function) == 0xFFFF)
                continue;

            /*printk("pci: Found %93$%3i %3i %3i%97$ - 0x%02x, 0x%02x\n", bus, device, function,
                   get_class(bus, device, function), get_subclass(bus, device, function));*/

            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%02x.%02x.%02x", bus, device, function);

            auto dev_device = new Dev::Device(buffer);

            dev_device->addAttribute(
                Dev::Device::attribute("device_id", get_device_id(bus, device, function)));
            dev_device->addAttribute(
                Dev::Device::attribute("vendor_id", get_vendor_id(bus, device, function)));

            dev_device->addAttribute(
                Dev::Device::attribute("class", get_class(bus, device, function)));
            dev_device->addAttribute(
                Dev::Device::attribute("subclass", get_subclass(bus, device, function)));
            dev_device->addAttribute(
                Dev::Device::attribute("prog_if", get_prog_if(bus, device, function)));

            fill_bars(bus, device, function, dev_device);

            dev_bus->addDevice(dev_device);
        }
    }

    uint16_t get_vendor_id(uint16_t bus, uint8_t device, uint8_t function) {
        return read_word(bus, device, function, 0x00);
    }

    uint16_t get_device_id(uint16_t bus, uint8_t device, uint8_t function) {
        return read_word(bus, device, function, 0x02);
    }

    uint8_t get_class(uint16_t bus, uint8_t device, uint8_t function) {
        return read_byte(bus, device, function, 0x0B);
    }

    uint8_t get_subclass(uint16_t bus, uint8_t device, uint8_t function) {
        return read_byte(bus, device, function, 0x0A);
    }

    uint8_t get_prog_if(uint16_t bus, uint8_t device, uint8_t function) {
        return read_byte(bus, device, function, 0x09);
    }
}