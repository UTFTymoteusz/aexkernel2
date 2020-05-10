#include "aex/dev/device.hpp"

#include "aex/dev/tree.hpp"
#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
    Device::Device(const char* name) {
        strncpy(this->name, name, sizeof(this->name));
    }

    Device::~Device() {
        // remove ourselves from the parent

        // children.~RCPArray();
    }

    void Device::addResource(Device::resource resource) {
        auto scopeLock = ScopeSpinlock(_lock);

        _resources.pushBack(resource);
    }

    optional<Device::resource> Device::getResource(int index) {
        auto scopeLock = ScopeSpinlock(_lock);

        if (index < 0 || index >= _resources.count())
            return {};

        return _resources.at(index);
    }

    void Device::registerDevice(Device* device) {
        device->parent = this;
        this->children.addRef(device);
    }
}