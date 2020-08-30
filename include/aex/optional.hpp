#pragma once

#include "aex/errno.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    template <typename T>
    struct optional {
        bool    has_value  = false;
        error_t error_code = ENONE;
        T       value;

        static optional error(error_t code) {
            auto opt       = optional();
            opt.error_code = code;
            return opt;
        }

        optional() {}

        optional(T value) : value(value) {
            has_value = true;
        }

        optional(error_t error) {
            error_code = error;
        }

        operator bool() {
            return has_value;
        }

        operator error_t() {
            return error_code;
        }
    };
}