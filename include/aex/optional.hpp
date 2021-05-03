#pragma once

#include "aex/errno.hpp"
#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX {
    template <typename T>
    struct API optional {
        bool    has_value = false;
        error_t error     = ENONE;
        T       value;

        optional() {}

        optional(T value) : value(value) {
            has_value = true;
        }

        optional(error_t a_error) {
            error = a_error;
        }

        optional(T value, error_t a_error) : value(value) {
            has_value = true;
            error     = a_error;
        }

        operator bool() {
            return has_value;
        }

        operator error_t() {
            return error;
        }

        // idk
        operator T() {
            return value;
        }
    };
}