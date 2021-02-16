#pragma once

#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Sys::ACPI {
    struct API sdt_header {
        char signature[4];

        uint32_t length;
        uint8_t  revision;
        uint8_t  checksum;

        char oem_id[6];
        char oem_tbl_id[8];

        uint32_t oem_revision;
        uint32_t creator_id;
        uint32_t creator_revision;
    } PACKED;
}