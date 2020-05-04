#include "aex/dev/device.hpp"

#include "aex/optional.hpp"
#include "aex/spinlock.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
    Device::Device(const char* name) {
        strncpy(this->name, name, sizeof(this->name));
    }

    void Device::addAttribute(Device::attribute attribute) {
        auto scopeLock = ScopeSpinlock(_lock);

        _attributes.pushBack(attribute);
    }

    optional<Device::attribute> Device::getAttribute(const char* name) {
        auto scopeLock = ScopeSpinlock(_lock);

        for (int i = 0; i < _attributes.count(); i++) {
            auto attribute = _attributes.at(i);

            if (strcmp(attribute.name, name) != 0)
                continue;

            return attribute;
        }

        return {};
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

    void print_attribute(Device::attribute attribute, int depth = 0) {
        for (int i = 0; i < depth; i++)
            printk("  ");

        switch (attribute.type) {
        case Device::attribute::INTEGER:
            printk("%s: (n) %li\n", attribute.name, attribute.integer);
            break;
        case Device::attribute::STRING:
            printk("%s: (s) %s\n", attribute.name, attribute.string);
            break;
        case Device::attribute::GROUP:
            printk("%s: (g)\n", attribute.name, attribute.string);

            for (int i = 0; i < attribute.group.count(); i++) {
                auto c_attribute = attribute.group.at(i);

                print_attribute(c_attribute, depth + 1);
            }
            break;
        default:
            break;
        }
    }

    void Device::printAttributesDebug() {
        auto scopeLock = ScopeSpinlock(_lock);

        for (int i = 0; i < _attributes.count(); i++) {
            auto attribute = _attributes.at(i);
            print_attribute(attribute);
        }
    }

    void Device::printResourcesDebug() {
        auto scopeLock = ScopeSpinlock(_lock);

        for (int i = 0; i < _resources.count(); i++) {
            auto resource = _resources.at(i);

            switch (resource.type) {
            case resource::type_t::MEMORY:
                printk("MEM: 0x%p - 0x%p\n", resource.start, resource.end);
                break;
            case resource::type_t::IO:
                printk("IO : 0x%p - 0x%p\n", resource.start, resource.end);
                break;
            default:
                break;
            }
        }
    }
}