#pragma once

#include <stdint.h>

namespace AEX {
    template <typename T>
    struct optional {
        bool    has_value  = false;
        int16_t error_code = 0;
        T       value;

        optional() {
            has_value = false;
        }

        optional(T value) : value(value) {
            has_value = true;
        }
    };
}