#pragma once

#include <stdint.h>

namespace AEX::Dev {
    typedef int32_t devid_t;

    class Device {
      public:
        enum type_t : uint8_t {
            BLOCK = 0,
            CHAR  = 1,
            NET   = 2,
        };

        type_t type;
        char   name[32];
        int    id;

        Device(const char* name, type_t type);

        /**
         * Registers a device.
         * @returns True if registration succeded.
         */
        bool registerDevice();
    };
}