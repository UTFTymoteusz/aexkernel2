#include "dev/driver/sata/ahci.hpp"

#include "aex/dev/tree.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"

#include "dev/driver/sata.hpp"

namespace AEX::Dev::SATA {
    AHCI::AHCI(void* addr, int index) {
        this->hba   = (hba_t*) addr;
        this->index = index;

        if (!bus_exists("sata"))
            new Bus("sata");

        hba->global_host_control |= 1 << 31; // Let's set it to AHCI mode just incase.

        command_slots = ((hba->host_capability >> 8) & 0b11111) + 1;
        printk("%i command slots\n", command_slots);

        scan_ports();

        printk(PRINTK_OK "sata: ahci%i initialized\n", index);
    }

    void AHCI::scan_ports() {
        uint32_t pi = hba->port_implemented;

        for (int i = 0; i < 32; i++) {
            if (!(pi & (1 << i)))
                continue;

            scan_port(&hba->ports[i], i);
        }
    }

    void AHCI::scan_port(hba_port_t* port, int port_index) {
        static int sr_counter = 0;
        static int sd_counter = 0;

        uint32_t status = port->sata_status;

        uint8_t power_management = (status >> 8) & 0b1111;
        uint8_t device_detection = (status >> 0) & 0b1111;

        if (power_management != POWER_MANAGEMENT_ACTIVE || device_detection != DETECTION_PRESENT)
            return;

        auto _device = new device();

        _device->hba_port        = port;
        _device->max_commands    = command_slots;
        _device->command_headers = (hba_command_header*) VMem::kernel_pagemap->allocContinuous(
            sizeof(hba_command_header) * command_slots, PAGE_WRITE);
        _device->command_tables = (hba_command_table*) VMem::kernel_pagemap->allocContinuous(
            8192 * command_slots, PAGE_WRITE);
        _device->fis =
            (hba_fis*) VMem::kernel_pagemap->allocContinuous(sizeof(hba_fis), PAGE_WRITE);

        port->command_list_address = VMem::kernel_pagemap->paddrof(_device->command_headers);
        port->fis_address          = VMem::kernel_pagemap->paddrof(_device->fis);

        int prdt_count = (8192 - sizeof(hba_command_table)) / sizeof(phys_region_table_entry);

        _device->max_page_burst = prdt_count - 2;

        for (int i = 0; i < command_slots; i++) {
            _device->command_headers[i].command_table_addr =
                VMem::kernel_pagemap->paddrof(&_device->command_tables[i]);

            _device->command_headers[i].phys_region_table_len = prdt_count;
        }

        SATADevice* sata_device;
        char        buffer[16];

        switch (port->signature) {
        case SATA_SIG_ATAPI:
            printk("ahci%i: port%i: Found SATAPI\n", index, port_index);

            snprintf(buffer, sizeof(buffer), "sr%i", sr_counter);
            sr_counter++;

            sata_device             = new SATADevice(buffer);
            sata_device->controller = this;

            register_device("sata", sata_device);
            break;
        case SATA_SIG_SEMB:
            printk("ahci%i: port%i: Found SEMB\n", index, port_index);
            break;
        case SATA_SIG_PM:
            printk("ahci%i: port%i: Found PM\n", index, port_index);
            break;
        default:
            printk("ahci%i: port%i: Found SATA\n", index, port_index);

            snprintf(buffer, sizeof(buffer), "sd%i", sd_counter);
            sd_counter++;

            sata_device             = new SATADevice(buffer);
            sata_device->controller = this;

            register_device("sata", sata_device);
            break;
        }
    }
}