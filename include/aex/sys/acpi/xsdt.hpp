#pragma once

#include "aex/sys/acpi/rsdt.hpp"
#include "aex/sys/acpi/sdt.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Sys::ACPI {
    struct API xsdp {
        rsdp m_rsdp;

        uint32_t length;
        uint64_t xsdt_address;
        uint8_t  extended_checksum;
        uint8_t  reserved[3];
    } PACKED;

    struct API xsdt {
        sdt_header header;
        uint64_t   table_pointers[];
    } PACKED;
}