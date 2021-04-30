#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::Tree {
    struct API resource {
        enum type_t : uint8_t {
            RES_MEMORY = 0,
            RES_IO     = 1,
            RES_IRQ    = 2,
        };

        type_t type;

        union {
            size_t start;
            size_t value;
        };
        size_t end;

        resource() {
            type  = RES_MEMORY;
            start = 0;
            end   = 0;
        }

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
