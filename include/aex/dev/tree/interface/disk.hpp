#pragma once

#include "aex/dev/tree/interface.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Dev::Tree {
    class Device;

    class Disk : public Interface {
        public:
        bool bind(Device* device);
    };
};