#pragma once

#include "aex/sys/acpi/sdt.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Sys::ACPI {
    struct rsdp {
        char     signature[8];
        uint8_t  checksum;
        char     oem_id[6];
        uint8_t  revision;
        uint32_t rsdt_address;
    } PACKED;

    struct rsdt {
        sdt_header header;
        uint32_t   table_pointers[];
    } PACKED;
}