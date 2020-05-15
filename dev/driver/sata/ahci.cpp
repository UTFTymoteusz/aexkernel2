#include "dev/driver/sata/ahci.hpp"

#include "aex/dev/tree/tree.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"

#include "dev/driver/sata/satadevice.hpp"

namespace AEX::Dev::SATA {
    AHCI::AHCI(void* addr, int index) {
        this->hba   = (hba_t*) addr;
        this->index = index;

        if (!Tree::bus_exists("sata"))
            new Tree::Bus("sata");

        hba->global_host_control |= 1 << 31; // Let's set it to AHCI mode just incase.

        command_slots = ((hba->host_capability >> 8) & 0b11111) + 1;
        printk("ahci%i: %i command slots\n", index, command_slots);

        scan_ports();

        printk(PRINTK_OK "ahci%i: Initialized\n", index);
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

        int  max_cmd = command_slots;
        auto headers = (hba_command_header*) VMem::kernel_pagemap->allocContinuous(
            sizeof(hba_command_header) * command_slots, PAGE_WRITE | PAGE_NOCACHE);
        auto tables = (hba_command_table*) VMem::kernel_pagemap->allocContinuous(
            8192 * command_slots, PAGE_WRITE | PAGE_NOCACHE);
        auto fis = (hba_fis*) VMem::kernel_pagemap->allocContinuous(sizeof(hba_fis),
                                                                    PAGE_WRITE | PAGE_NOCACHE);

        memset(headers, '\0', sizeof(hba_command_header) * command_slots);
        memset(tables, '\0', 8192 * command_slots);
        memset(fis, '\0', sizeof(hba_fis));

        // Forgetting these made me put away this goddamned code for 3 days
        fis->dma_setup.fis_type = fis_type::DMA_SETUP;
        fis->pio_setup.fis_type = fis_type::PIO_SETUP;
        fis->reg_d2h.fis_type   = fis_type::REG_D2H;
        fis->dev_bits[0]        = fis_type::DEV_BITS;

        port->command_list_address = VMem::kernel_pagemap->paddrof(headers);
        port->fis_address          = VMem::kernel_pagemap->paddrof(fis);

        port->interrupt_enable = 0x00000000;
        port->interrupt_status = 0xFFFFFFFF;
        port->sata_error       = 0xFFFFFFFF;

        int prdt_count = (8192 - sizeof(hba_command_table)) / sizeof(phys_region_table_entry);

        int max_page_burst = prdt_count - 2;
        int max_prts       = prdt_count;

        for (int i = 0; i < command_slots; i++) {
            headers[i].command_table_addr =
                VMem::kernel_pagemap->paddrof((void*) ((size_t) tables + 8192 * i));

            headers[i].phys_region_table_len = prdt_count;
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
            sata_device->type       = type_t::SATAPI;
            sata_device->atapi      = true;

            sata_device->max_commands   = max_cmd;
            sata_device->hba_port       = port;
            sata_device->max_page_burst = max_page_burst;
            sata_device->max_prts       = max_prts;

            sata_device->command_headers = headers;
            sata_device->command_tables  = tables;

            sata_device->init();

            Tree::register_device("sata", sata_device);
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
            sata_device->type       = type_t::SATA;
            sata_device->atapi      = false;

            sata_device->max_commands   = max_cmd;
            sata_device->hba_port       = port;
            sata_device->max_page_burst = max_page_burst;
            sata_device->max_prts       = max_prts;

            sata_device->command_headers = headers;
            sata_device->command_tables  = tables;

            sata_device->init();

            register_device("sata", sata_device);
            break;
        }
    }
}