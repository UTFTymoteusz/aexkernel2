#pragma once

#include "aex/mem.hpp"
#include "aex/string.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::ACPI {
    struct acpi_table {};

    extern uint8_t revision;

    bool validate_table(const void* tbl, size_t len);

    acpi_table* find_table(const char signature[4], int index);
}