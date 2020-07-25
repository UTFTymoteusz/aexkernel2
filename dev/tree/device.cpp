#include "aex/dev/tree/device.hpp"

#include "aex/dev/tree.hpp"
#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev::Tree {
    Device::Device(const char* name, Device* parent) {
        strncpy(this->name, name, sizeof(this->name));

        if (!parent && root_device)
            parent = root_device;

        if (!parent)
            return;

        this->parent = parent;
        this->parent->children.addRef(this);
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