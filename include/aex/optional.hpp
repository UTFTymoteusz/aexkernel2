#pragma once

#include "aex/errno.hpp"

#include <stdint.h>

namespace AEX {
    template <typename T>
    struct optional {
        bool    has_value  = false;
        error_t error_code = error_t::ENONE;
        T       value;

        static optional error(error_t code) {
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