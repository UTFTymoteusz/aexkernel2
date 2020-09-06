#include "aex/sys/acpi.hpp"

#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::ACPI {
    Mem::Vector<acpi_table*> tables;
    uint8_t                  revision;

    bool add_table(acpi_table* table) {
        auto header = (sdt_header*) table;
        char buffer[5];

        buffer[4] = '\0';

        memcpy((void*) buffer, (void*) header->signature, 4);

        if (!validate_table(table, header->length)) {
            printk(PRINTK_WARN "acpi: Found table %s, but its checksum wasn't right\n", buffer);
            return false;
        }

        tables.pushBack(table);

        printk("acpi: Found table %s, len %i\n", buffer, header->length);

        return true;
    }

    void _init() {
        auto m_fadt = (fadt*) find_table("FACP", 0);
        if (!m_fadt)
            kpanic("This system has no FADT, what the hell?");

        auto table_hdr =
            (sdt_header*) Mem::kernel_pagemap->map(sizeof(sdt_header), m_fadt->dsdt, 0);
        auto table = (acpi_table*) Mem::kernel_pagemap->map(table_hdr->length, m_fadt->dsdt, 0);

        Mem::kernel_pagemap->free(table_hdr, sizeof(sdt_header));

        add_table(table);

        if (m_fadt->pm_timer_block == 0)
            kpanic("acpi: no power management timer :(\n");
    }

    void init() {
        printk(PRINTK_INIT "acpi: Initializing\n");

        auto xsdp = find_xsdp();
        if (xsdp != nullptr) {
            printk("acpi: Found the xdsp\n");

            auto m_xsdt = (xsdt*) Mem::kernel_pagemap->map(xsdp->length, xsdp->xsdt_address, 0);

            if (!add_table((acpi_table*) m_xsdt)) {
                printk("acpi: Failed\n");
                return;
            }

            revision = m_xsdt->header.revision;

            for (size_t i = sizeof(xsdt); i < m_xsdt->header.length; i += 8) {
                uint64_t addr = *((uint64_t*) ((size_t) m_xsdt + i));
                auto     table_hdr =
                    (sdt_header*) Mem::kernel_pagemap->map(sizeof(sdt_header), addr, 0);
                auto table = (acpi_table*) Mem::kernel_pagemap->map(table_hdr->length, addr, 0);

                Mem::kernel_pagemap->free(table_hdr, sizeof(sdt_header));

                add_table(table);
            }

            _init();

            printk(PRINTK_OK "acpi: Initialized\n");
            return;
        }

        auto rsdp = find_rsdp();
        if (rsdp != nullptr) {
            printk("acpi: Found the RSDP\n");

            auto m_rsdt = (rsdt*) Mem::kernel_pagemap->map(4096, rsdp->rsdt_address, 0);

            if (!add_table((acpi_table*) m_rsdt)) {
                printk("acpi: Failed\n");
                return;
            }

            revision = m_rsdt->header.revision;

            for (size_t i = sizeof(rsdt); i < m_rsdt->header.length; i += 4) {
                uint32_t addr  = *((uint32_t*) ((size_t) m_rsdt + i));
                auto     table = (acpi_table*) Mem::kernel_pagemap->map(4096, addr, 0);

                add_table(table);
            }

            _init();

            printk(PRINTK_OK "acpi: Initialized\n");
            return;
        }

        printk("acpi: Failed\n");
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

    void* MADT::findEntry(int type, int index) {
        for (size_t i = 0; i < header.length - sizeof(ACPI::MADT);) {
            auto entry = (ACPI::MADT::entry*) &(data[i]);

            if (entry->type != type) {
                i += entry->len;
                continue;
            }

            if (index > 0) {
                index--;
                i += entry->len;

                continue;
            }

            return (void*) entry;
        }

        return nullptr;
    }
}