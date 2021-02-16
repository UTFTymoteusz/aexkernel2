#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev {
    typedef int32_t devid_t;

    enum dev_type_t : uint8_t {
        DEV_BLOCK = 0,
        DEV_CHAR  = 1,
        DEV_NET   = 2,
    };

    class API Device {
        public:
        dev_type_t type;

        char name[32];
        int  id;

        Device(const char* name, dev_type_t type);

        /**
         * Registers the device.
         * @returns True if registration has succeded.
         **/
        bool registerDevice();
    };
}