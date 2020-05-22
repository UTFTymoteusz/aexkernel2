#pragma once

#include <stdint.h>

namespace AEX::Dev {
    class Device {
      public:
        enum type_t : uint8_t {
            BLOCK = 0,
            CHAR  = 1,
            NET   = 2,
        };

        type_t type;

        Device(type_t type) : type(type) {}
    };
}