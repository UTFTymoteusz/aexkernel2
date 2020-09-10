#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::Tree {

    struct resource {
        enum type_t : uint8_t {
            MEMORY = 0,
            IO     = 1,
            IRQ    = 2,
        };

        type_t type;

        union {
            size_t start;
            size_t value;
        };
        size_t end;

        resource() {}

        resource(type_t type, size_t value) {
            this->type  = type;
            this->value = value;
            this->end   = value;
        }

        resource(type_t type, size_t start, size_t end) {
            this->type  = type;
            this->start = start;
            this->end   = end;
        }
    };
}
