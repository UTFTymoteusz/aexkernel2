#include "aex/sys/acpi.hpp"

#include "aex/assert.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"
#include "aex/sys/acpi/fadt.hpp"
#include "aex/sys/acpi/rsdt.hpp"
#include "aex/sys/acpi/sdt.hpp"
#include "aex/sys/acpi/xsdt.hpp"

#include "sys/acpi.hpp"

namespace AEX::Sys::ACPI {
    Mem::Vector<acpi_table*> tables;
    uint8_t                  revision;

    bool add_table(acpi_table* table) {
        auto header    = (sdt_header*) table;
        char buffer[8] = {};

        memcpy((void*) buffer, (void*) header->signature, 4);

        if (!validate_table(table, header->length)) {
            printk(PRINTK_WARN "acpi: Found table %s, but its checksum wasn't right\n", buffer);
            return false;
        }

        tables.push(table);

        printk("acpi: Found table %s, len %i\n", buffer, header->length);

        return true;
    }

    void facp_init();

    void init() {
        printk(PRINTK_INIT "acpi: Initializing\n");

        auto xsdp = find_xsdp();
        if (xsdp != nullptr) {
            printk("acpi: Found the XDSP\n");

            auto _xsdt = (xsdt*) Mem::kernel_pagemap->map(xsdp->length, xsdp->xsdt_address, 0);

            if (!add_table((acpi_table*) _xsdt)) {
                printk("acpi: Failed\n");
                return;
            }

            revision = _xsdt->header.revision;

            for (size_t i = sizeof(xsdt); i < _xsdt->header.length; i += 8) {
                uint64_t addr = *((uint64_t*) ((size_t) _xsdt + i));
                auto     table_hdr =
                    (sdt_header*) Mem::kernel_pagemap->map(sizeof(sdt_header), addr, 0);
                auto table = (acpi_table*) Mem::kernel_pagemap->map(table_hdr->length, addr, 0);

                Mem::kernel_pagemap->free(table_hdr, sizeof(sdt_header));

                add_table(table);
            }

            facp_init();

            printk(PRINTK_OK "acpi: Initialized\n");
            return;
        }

        auto rsdp = find_rsdp();
        if (rsdp != nullptr) {
            printk("acpi: Found the RSDP\n");

            auto _rsdt = (rsdt*) Mem::kernel_pagemap->map(4096, rsdp->rsdt_address, 0);

            if (!add_table((acpi_table*) _rsdt)) {
                printk("acpi: Failed\n");
                return;
            }

            revision = _rsdt->header.revision;

            for (size_t i = sizeof(rsdt); i < _rsdt->header.length; i += 4) {
                uint32_t addr  = *((uint32_t*) ((size_t) _rsdt + i));
                auto     table = (acpi_table*) Mem::kernel_pagemap->map(4096, addr, 0);

                add_table(table);
            }

            facp_init();

            printk(PRINTK_OK "acpi: Initialized\n");
            return;
        }

        printk("acpi: Failed\n");
    }

    void facp_init() {
        auto _fadt = (fadt*) find_table("FACP", 0);
        AEX_ASSERT(_fadt);

        auto table_hdr = (sdt_header*) Mem::kernel_pagemap->map(sizeof(sdt_header), _fadt->dsdt, 0);
        auto table     = (acpi_table*) Mem::kernel_pagemap->map(table_hdr->length, _fadt->dsdt, 0);

        Mem::kernel_pagemap->free(table_hdr, sizeof(sdt_header));

        add_table(table);

        AEX_ASSERT(_fadt->pm_timer_block);
    }

    bool validate_table(const void* tbl, size_t len) {
        uint8_t* m_tbl = (uint8_t*) tbl;
        uint8_t  sum   = 0;

        for (size_t i = 0; i < len; i++)
            sum += m_tbl[i];

        return sum == 0;
    }

    acpi_table* find_table(const char signature[4], int index) {
        for (int i = 0; i < tables.count(); i++) {
            auto table = tables.at(i);

            if (memcmp((void*) signature, (void*) ((sdt_header*) table), 4) != 0) {
                continue;
            }

            if (index > 0) {
                index--;
                continue;
            }

            return (acpi_table*) table;
        }

        return nullptr;
    };
}