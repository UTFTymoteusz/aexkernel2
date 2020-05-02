#pragma once

namespace AEX {
    template <typename T>
    struct optional {
        bool has_value = false;
        T    value;

        optional() {
            has_value = false;
        }

        optional(T value) : value(value) {
            has_value = true;
        }
    };
}