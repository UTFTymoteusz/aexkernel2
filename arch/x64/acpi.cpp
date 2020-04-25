#include "kernel/acpi/acpi.hpp"

#include "kernel/string.hpp"
#include "mem/pmem.hpp"
#include "mem/vmem.hpp"

#include <stddef.h>

namespace AEX::ACPI {
    void* _find_rsdptr(size_t start, size_t length) {
        for (size_t i = start; i < start + length; i += 0x10) {
            void* ptr = (void*) i;

            if (memcmp(ptr, "RSD PTR ", 8) == 0)
                return ptr;
        }

        return nullptr;
    }

    void* find_rsdptr() {
        void* rsdptr;

        rsdptr = _find_rsdptr(0x00, 1024);
        if (rsdptr)
            return rsdptr;

        rsdptr = _find_rsdptr(0x000E0000, 131071);
        if (rsdptr)
            return rsdptr;

        return nullptr;
    }

    rsdp_t* find_rsdp() {
        auto rsdp = (rsdp_t*) VMem::kernel_pagemap->map(
            sizeof(rsdp_t), (PMem::phys_addr) find_rsdptr(), PAGE_WRITE);
        auto xsdp = (xsdp_t*) rsdp;

        if (validate_table((void*) xsdp, sizeof(xsdp_t)))
            return nullptr;

        if (!validate_table((void*) rsdp, sizeof(rsdp_t)))
            return nullptr;

        return rsdp;
    }

    xsdp_t* find_xsdp() {
        auto xsdp = (xsdp_t*) VMem::kernel_pagemap->map(
            sizeof(xsdp_t), (PMem::phys_addr) find_rsdptr(), PAGE_WRITE);

        if (!validate_table((void*) xsdp, sizeof(xsdp_t))) {
            // VMem::kernel_pagemap->unmap(xsdp);
            return nullptr;
        }

        return xsdp;
    }
}