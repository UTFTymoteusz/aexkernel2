#include "kernel/acpi/acpi.hpp"

#include "aex/mem/pmem.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/string.hpp"

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

    rsdp* find_rsdp() {
        void* rsd_ptr = find_rsdptr();
        if (!rsd_ptr)
            return nullptr;

        auto _rsdp =
            (rsdp*) VMem::kernel_pagemap->map(sizeof(rsdp), (PMem::phys_addr) rsd_ptr, PAGE_WRITE);
        auto _xsdp = (xsdp*) _rsdp;

        if (validate_table((void*) _xsdp, sizeof(xsdp)) && _xsdp->xsdt_address != 0x0000)
            return nullptr;

        if (!validate_table((void*) _rsdp, sizeof(rsdp)))
            return nullptr;

        if (_rsdp->rsdt_address == 0x0000)
            return nullptr;

        return _rsdp;
    }

    xsdp* find_xsdp() {
        void* rsd_ptr = find_rsdptr();
        if (!rsd_ptr)
            return nullptr;

        auto _xsdp =
            (xsdp*) VMem::kernel_pagemap->map(sizeof(xsdp), (PMem::phys_addr) rsd_ptr, PAGE_WRITE);

        if (!validate_table((void*) _xsdp, sizeof(xsdp))) {
            // VMem::kernel_pagemap->unmap(xsdp);
            return nullptr;
        }

        if (_xsdp->xsdt_address == 0x0000)
            return nullptr;

        return _xsdp;
    }
}