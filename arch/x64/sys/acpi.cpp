#include "aex/sys/acpi.hpp"

#include "aex/mem.hpp"
#include "aex/string.hpp"
#include "aex/sys/acpi/rsdt.hpp"
#include "aex/sys/acpi/xsdt.hpp"

namespace AEX::Sys::ACPI {
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

        auto _rsdp = (rsdp*) Mem::kernel_pagemap->map(sizeof(rsdp), (Mem::Phys::phys_addr) rsd_ptr,
                                                      PAGE_WRITE);
        auto _xsdp = (xsdp*) _rsdp;

        if (validate_table((void*) _xsdp, sizeof(xsdp)) && _xsdp->xsdt_address != 0x0000) {
            Mem::kernel_pagemap->free(_rsdp, sizeof(rsdp));
            return nullptr;
        }

        if (!validate_table((void*) _rsdp, sizeof(rsdp))) {
            Mem::kernel_pagemap->free(_rsdp, sizeof(rsdp));
            return nullptr;
        }

        if (_rsdp->rsdt_address == 0x0000) {
            Mem::kernel_pagemap->free(_rsdp, sizeof(rsdp));
            return nullptr;
        }

        return _rsdp;
    }

    xsdp* find_xsdp() {
        void* rsd_ptr = find_rsdptr();
        if (!rsd_ptr)
            return nullptr;

        auto _xsdp = (xsdp*) Mem::kernel_pagemap->map(sizeof(xsdp), (Mem::Phys::phys_addr) rsd_ptr,
                                                      PAGE_WRITE);

        if (!validate_table((void*) _xsdp, sizeof(xsdp))) {
            Mem::kernel_pagemap->free(_xsdp, sizeof(xsdp));
            return nullptr;
        }

        if (_xsdp->xsdt_address == 0x0000) {
            Mem::kernel_pagemap->free(_xsdp, sizeof(xsdp));
            return nullptr;
        }

        return _xsdp;
    }
}