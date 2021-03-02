#pragma once

#include <stdint.h>

namespace AEX::Sys::PCI {
    const char* resolve(uint16_t _class, uint16_t subclass, uint16_t prog_if);
}