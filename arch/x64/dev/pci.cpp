#include "aex/dev/pci.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/dev/tree/bus.hpp"
#include "aex/dev/tree/device.hpp"
#include "aex/dev/tree/tree.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/module.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

#include <stdint.h>

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

namespace AEX::Dev::PCI {
    void scan_all_buses();

    Tree::Bus* dev_bus;

    class PCI : public Tree::Driver {
        public:
        PCI() : Driver("pci") {}
        ~PCI() {}

        bool check(Tree::Device* device) {
            if (strcmp(device->name, "pci") != 0)
                return false;

            return true;
        }

        void bind(Tree::Device*) {
            printk(PRINTK_INIT "pci: Initializing\n");

            dev_bus = new Tree::Bus("pci");
            scan_all_buses();

            printk(PRINTK_OK "pci: Initialized\n");
        }
    };

    struct address {
        uint8_t bus;
        uint8_t device;
        uint8_t function;

        address(uint8_t bus, uint8_t device, uint8_t function) {
            this->bus      = bus;
            this->device   = device;
            this->function = function;
        }

        address(size_t value) {
            this->bus      = (value >> 16) & 0xFF;
            this->device   = (value >> 8) & 0xFF;
            this->function = (value >> 0) & 0xFF;
        }

        size_t get_value() {
            return (((size_t) this->bus) << 16) | (((size_t) this->device) << 8) | this->function;
        }
    };

    PCI* driver;

    Spinlock lock;

    void init() {
        driver = new PCI();
        Dev::Tree::register_driver("main", driver);
    }

    int PCIDevice::getIRQ() {
        uint8_t (*set_pin)(uint8_t, uint8_t, uint8_t, uint8_t);

        while (!(set_pin = (typeof(set_pin)) get_global_symbol("acpi_set_pci_pin")))
            Proc::Thread::sleep(250);

        return set_pin(bus, device, function, interrupt_pin);
    }

    uint16_t read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
    uint8_t  read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

    void scan_all_buses();
    void scan_device(uint16_t bus, uint8_t device);

    uint16_t get_vendor_id(uint16_t bus, uint8_t device, uint8_t function);
    uint16_t get_device_id(uint16_t bus, uint8_t device, uint8_t function);

    uint8_t get_class(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_subclass(uint16_t bus, uint8_t device, uint8_t function);
    uint8_t get_prog_if(uint16_t bus, uint8_t device, uint8_t function);

    uint8_t get_interrupt_pin(uint16_t bus, uint8_t device, uint8_t function);
    void    set_interrupt_pin(uint16_t bus, uint8_t device, uint8_t function, uint8_t pin);

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

    void write_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t val) {
        uint32_t address = (uint32_t)((uint32_t) bus << 16) | ((uint32_t) device << 11) |
                           ((uint32_t) function << 8) | (offset & 0xFC) | (1 << 31);

        auto scopeLock = ScopeSpinlock(lock);

        Sys::CPU::outportd(CONFIG_ADDRESS, address);

        uint32_t data = (uint32_t) Sys::CPU::inportd(CONFIG_DATA);
        data &= ~(0xFF << ((offset & 3) * 8));
        data |= (val << ((offset & 3) * 8));

        Sys::CPU::outportd(CONFIG_DATA, data);
    }

    void write_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t val) {
        uint32_t address = (uint32_t)((uint32_t) bus << 16) | ((uint32_t) device << 11) |
                           ((uint32_t) function << 8) | (offset & 0xFC) | (1 << 31);

        auto scopeLock = ScopeSpinlock(lock);

        Sys::CPU::outportd(CONFIG_ADDRESS, address);

        uint32_t data = (uint32_t) Sys::CPU::inportd(CONFIG_DATA);
        data &= ~(0xFFFF << ((offset & 2) * 8));
        data |= (val << ((offset & 2) * 8));

        Sys::CPU::outportd(CONFIG_DATA, data);
    }

    void write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t val) {
        uint32_t address = (uint32_t)((uint32_t) bus << 16) | ((uint32_t) device << 11) |
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

    void fill_bars(uint8_t bus, uint8_t device, uint8_t function, PCIDevice* dev_device) {
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
                auto resource = Tree::Device::resource();

                resource.type  = Tree::Device::resource::type_t::IO;
                resource.value = addr;
                resource.end   = addr + len;

                dev_device->addResource(resource);
            }
            else {
                auto resource = Tree::Device::resource();

                resource.type  = Tree::Device::resource::type_t::MEMORY;
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

        int int_pin = 1;

        for (uint8_t function = 0; function < 8; function++) {
            if (get_vendor_id(bus, device, function) == 0xFFFF)
                continue;

            /*printk("pci: Found %93$%3i %3i %3i%$ - 0x%02x, 0x%02x\n", bus, device, function,
                   get_class(bus, device, function), get_subclass(bus, device, function));*/

            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%02x.%02x.%02x", bus, device, function);

            auto dev_device = new PCIDevice(buffer);

            dev_device->bus      = bus;
            dev_device->device   = device;
            dev_device->function = function;

            dev_device->device_id = get_device_id(bus, device, function);
            dev_device->vendor_id = get_vendor_id(bus, device, function);

            dev_device->p_class  = get_class(bus, device, function);
            dev_device->subclass = get_subclass(bus, device, function);
            dev_device->prog_if  = get_prog_if(bus, device, function);

            dev_device->interrupt_pin = 0;

            fill_bars(bus, device, function, dev_device);

            if (get_interrupt_pin(bus, device, function)) {
                set_interrupt_pin(bus, device, function, int_pin);

                dev_device->interrupt_pin = int_pin;

                int_pin++;
                if (int_pin > 4)
                    int_pin = 1;
            }

            dev_bus->registerDevice(dev_device);
        }
    }

    void set_busmaster(Tree::Device* _device, bool on) {
        auto pci_device = (PCIDevice*) _device;

        uint8_t bus      = pci_device->bus;
        uint8_t device   = pci_device->device;
        uint8_t function = pci_device->function;

        if (on)
            write_dword(bus, device, function, 0x04,
                        read_dword(bus, device, function, 0x04) | (1 << 2));
        else
            write_dword(bus, device, function, 0x04,
                        read_dword(bus, device, function, 0x04) & ~(1 << 2));
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

    uint8_t get_interrupt_pin(uint16_t bus, uint8_t device, uint8_t function) {
        return read_byte(bus, device, function, 0x3C);
    }

    void set_interrupt_pin(uint16_t bus, uint8_t device, uint8_t function, uint8_t pin) {
        write_byte(bus, device, function, 0x3C, pin);
    }
}