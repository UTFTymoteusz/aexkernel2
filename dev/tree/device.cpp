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

    void Device::add(resource resource) {
        SCOPE(m_lock);

        m_resources.push(resource);
    }

    optional<resource> Device::get(int index) {
        SCOPE(m_lock);

        if (index < 0 || index >= m_resources.count())
            return {};

        return m_resources.at(index);
    }

    void Device::registerDevice(Device* device) {
        device->parent = this;
        this->children.addRef(device);
    }
}