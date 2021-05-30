#include "aex/dev/tree/driver.hpp"

#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::Dev::Tree {
    Driver::Driver(const char* name) {
        strlcpy(this->name, name, sizeof(this->name));
    }

    Driver::~Driver() {}

    void register_base_drivers() {}
}