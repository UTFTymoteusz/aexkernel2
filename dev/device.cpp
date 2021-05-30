#include "aex/dev/device.hpp"

#include "aex/dev.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
    Device::Device(const char* name, dev_type_t type) : type(type) {
        strlcpy(this->name, name, sizeof(this->name));
    }

    bool Device::registerDevice() {
        this->id = devices.addRef(this);
        if (this->id == -1)
            return false;

        const char* type = "unknown";

        switch (this->type) {
        case DEV_BLOCK:
            type = "block";
            break;
        case DEV_CHAR:
            type = "char";
            break;
        case DEV_NET:
            type = "net";
            break;
        default:
            break;
        }

        printk("dev: Registered %s device '%s'\n", type, name);

        return true;
    }
}