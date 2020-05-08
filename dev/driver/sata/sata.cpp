#include "aex/dev/device.hpp"
#include "aex/dev/driver.hpp"
#include "aex/dev/pci.hpp"
#include "aex/dev/tree.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/mem/vmem.hpp"

#include "dev/driver/sata/ahci.hpp"

#include <stdint.h>

namespace AEX::Dev::SATA {
    class SATA : public Driver {
      public:
        SATA() : Driver("sata") {}
        ~SATA() {}

        bool check(Device* device) {
            auto pci_device = (PCI::PCIDevice*) device;

            if (pci_device->p_class != 0x01 || pci_device->subclass != 0x06 ||
                pci_device->prog_if != 0x01)
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

            void* addr = VMem::kernel_pagemap->map(0x1000, paddr, PAGE_NOCACHE | PAGE_WRITE);

            new AHCI(addr, index);
            index++;
        }

      private:
        int index = 0;
    };

    extern void sd_init();
    extern void sr_init();

    void init() {
        auto sata = new SATA();

        if (!bus_exists("sata"))
            new Bus("sata");

        if (!register_driver("pci", sata))
            printk(PRINTK_WARN "sata: Failed to register the PCI driver\n");

        sd_init();
        sr_init();
    }
}