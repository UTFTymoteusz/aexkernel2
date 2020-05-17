#pragma once

#include <stdint.h>

namespace AEX {
    template <typename T>
    struct optional {
        bool    has_value  = false;
        int16_t error_code = 0;
        T       value;

        static optional error(int code) {
            auto opt       = optional();
            opt.error_code = code;
            return opt;
        }

        optional() {
            has_value = false;
        }

        optional(T value) : value(value) {
            has_value = true;
        }
    };
}