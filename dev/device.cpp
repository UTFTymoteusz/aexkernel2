#include "aex/dev/device.hpp"

#include "aex/dev/dev.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
    Device::Device(const char* name, Device::type_t type) : type(type) {
        strncpy(this->name, name, sizeof(this->name));

        this->id = devices.addRef(this);
    }
}