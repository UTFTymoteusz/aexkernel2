#pragma once

namespace AEX {
    inline int8_t fromBCD(int8_t val) {
        return ((val & 0xF0) >> 1) + ((val & 0xF0) >> 3) + (val & 0x0F);
    }
}