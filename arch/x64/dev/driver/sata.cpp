#include "aex/dev/device.hpp"
#include "aex/dev/driver.hpp"
#include "aex/dev/pci.hpp"
#include "aex/dev/tree.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/mem/vmem.hpp"

namespace AEX::Dev::SATA {
    class AHCI {
      public:
        AHCI(void* addr, int index) {
            this->addr  = addr;
            this->index = index;

            if (!bus_exists("sata"))
                new Bus("sata");

            printk(PRINTK_OK "sata: ahci%i initialized\n", index);
        }

      private:
        void* addr;
        int   index;
    };

    class SATA : public Driver {
      public:
        SATA() : Driver("sata") {}
        ~SATA() {}

        bool check(Device* device) {
            auto pci_class = device->getAttribute("class");
            if (!pci_class.has_value || pci_class.value.integer != 0x01)
                return false;

            auto pci_subclass = device->getAttribute("subclass");
            if (!pci_subclass.has_value || pci_subclass.value.integer != 0x06)
                return false;

            auto pci_prog_if = device->getAttribute("prog_if");
            if (!pci_prog_if.has_value || pci_prog_if.value.integer != 0x01)
                return false;

            return true;
        }

        void bind(Device* device) {
            PCI::set_busmaster(device, true);

            PMem::phys_addr paddr = 0;

            for (int i = 5; i >= 0; i--) {
                auto resource = device->getResource(i);
                if (!resource.has_value || resource.value.type != Device::resource::type_t::MEMORY)
                    continue;

                paddr = resource.value.start;
                break;
            }

            void* addr = VMem::kernel_pagemap->map(0x1000, paddr, PAGE_WRITE);

            auto ahci = new AHCI(addr, index);
            index++;
        }

      private:
        int index = 0;
    };

    void init() {
        auto sata = new SATA();

        register_driver("pci", sata);
    }
}