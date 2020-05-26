#include "aex/dev/device.hpp"

#include "aex/dev/dev.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
    Device::Device(const char* name, Device::type_t type) : type(type) {
        strncpy(this->name, name, sizeof(this->name));
    }

    bool Device::registerDevice() {
        this->id = devices.addRef(this);
        if (this->id != -1) {
            const char* type = "unknown";

            switch (this->type) {
            case type_t::BLOCK:
                type = "block";
                break;
            case type_t::CHAR:
                type = "char";
                break;
            case type_t::NET:
                type = "net";
                break;
            default:
                break;
            }

            printk("dev: Registered %s device '%s'\n", type, name);
        }

        return this->id != -1;
    }
}