#include "aex/dev/interface.hpp"

#include "aex/dev/tree.hpp"
#include "aex/string.hpp"

namespace AEX::Dev {
    Interface::Interface(const char* name) {
        strncpy(this->name, name, sizeof(this->name));

        interfaces.addRef(this);
        printk("dev: Registered interface '%s'\n", this->name);
    }

    Interface::~Interface() {}

    bool Interface::bind(Device*) {
        return false;
    }

    extern void register_disk();

    void register_base_interfaces() {
        register_disk();
    }
}