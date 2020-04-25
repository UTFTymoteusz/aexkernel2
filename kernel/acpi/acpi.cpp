#include "kernel/acpi/acpi.hpp"

#include "kernel/printk.hpp"
#include "kernel/string.hpp"
#include "lib/rcparray.hpp"
#include "mem/pmem.hpp"
#include "mem/vmem.hpp"

#include <stddef.h>

namespace AEX::ACPI {
    RCPArray<table_t> tables;

    bool add_table(table_t* table) {
        auto header = (sdt_header_t*) table;
        char buffer[5];

        buffer[4] = '\0';

        memcpy((void*) buffer, (void*) header->signature, 4);

        if (!validate_table(table, header->length)) {
            printk(PRINTK_WARN "acpi: Found table %s, but its checksum wasn't right\n", buffer);
            return false;
        }

        tables.addRef(table);

        printk("acpi: Found table %s\n", buffer, table);

        return true;
    }

    void init() {
        printk(PRINTK_INIT "acpi: Initializing\n");

        auto xsdp = find_xsdp();
        if (xsdp != nullptr) {
            printk("acpi: Found the xdsp\n");

            auto xsdt = (xsdt_t*) VMem::kernel_pagemap->map(4096, xsdp->xsdt_address, 0);

            if (!add_table((table_t*) xsdt)) {
                printk("acpi: Failed\n");
                return;
            }

            for (size_t i = sizeof(xsdt_t); i < xsdt->header.length; i += 8) {
                uint64_t addr  = *((uint64_t*) ((size_t) xsdt + i));
                auto     table = (table_t*) VMem::kernel_pagemap->map(4096, addr, 0);

                add_table(table);
            }

            printk(PRINTK_OK "acpi: Initialized\n");
            return;
        }

        auto rsdp = find_rsdp();
        if (rsdp != nullptr) {
            printk("acpi: Found the RSDP\n");

            auto rsdt = (rsdt_t*) VMem::kernel_pagemap->map(4096, rsdp->rsdt_address, 0);

            if (!add_table((table_t*) rsdt)) {
                printk("acpi: Failed\n");
                return;
            }

            for (size_t i = sizeof(rsdt_t); i < rsdt->header.length; i += 4) {
                uint32_t addr  = *((uint32_t*) ((size_t) rsdt + i));
                auto     table = (table_t*) VMem::kernel_pagemap->map(4096, addr, 0);

                add_table(table);
            }

            printk(PRINTK_OK "acpi: Initialized\n");

            return;
        }

        printk("acpi: Failed\n");
    }

    bool validate_table(const void* tbl, size_t len) {
        uint8_t* _tbl = (uint8_t*) tbl;
        uint8_t  sum  = 0;

        for (size_t i = 0; i < len; i++)
            sum += _tbl[i];

        return sum == 0;
    }

    table_t* find_table(const char signature[4], int index) {
        for (int i = 0; i < tables.count(); i++) {
            auto table = tables.get(i);
            if (!table.isPresent())
                continue;

            if (memcmp((void*) signature, (void*) ((sdt_header_t*) &*table), 4) != 0) {
                continue;
            }

            if (index > 0) {
                index--;
                continue;
            }

            return &*table;
        }

        return nullptr;
    }
}