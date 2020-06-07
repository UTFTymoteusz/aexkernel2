#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::ACPI {
    typedef void* acpi_table;

    extern uint8_t revision;

    acpi_table* find_table(const char signature[4], int index);
}