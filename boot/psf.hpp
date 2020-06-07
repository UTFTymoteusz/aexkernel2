#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    struct psf1 {
        uint16_t magic;
        uint8_t  mode;
        uint8_t  size;

        uint8_t data[];
    };
}