#include "aex/sys/acpi.hpp"

#include "aex/mem.hpp"
#include "aex/string.hpp"

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

        auto m_rsdp = (rsdp*) Mem::kernel_pagemap->map(sizeof(rsdp), (Mem::Phys::phys_addr) rsd_ptr,
                                                       PAGE_WRITE);
        auto m_xsdp = (xsdp*) m_rsdp;

        if (validate_table((void*) m_xsdp, sizeof(xsdp)) && m_xsdp->xsdt_address != 0x0000) {
            Mem::kernel_pagemap->free(m_rsdp, sizeof(rsdp));
            return nullptr;
        }

        if (!validate_table((void*) m_rsdp, sizeof(rsdp))) {
            Mem::kernel_pagemap->free(m_rsdp, sizeof(rsdp));
            return nullptr;
        }

        if (m_rsdp->rsdt_address == 0x0000) {
            Mem::kernel_pagemap->free(m_rsdp, sizeof(rsdp));
            return nullptr;
        }

        return m_rsdp;
    }

    xsdp* find_xsdp() {
        void* rsd_ptr = find_rsdptr();
        if (!rsd_ptr)
            return nullptr;

        auto m_xsdp = (xsdp*) Mem::kernel_pagemap->map(sizeof(xsdp), (Mem::Phys::phys_addr) rsd_ptr,
                                                       PAGE_WRITE);

        if (!validate_table((void*) m_xsdp, sizeof(xsdp))) {
            Mem::kernel_pagemap->free(m_xsdp, sizeof(xsdp));
            return nullptr;
        }

        if (m_xsdp->xsdt_address == 0x0000) {
            Mem::kernel_pagemap->free(m_xsdp, sizeof(xsdp));
            return nullptr;
        }

        return m_xsdp;
    }
}