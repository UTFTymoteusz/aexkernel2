#include "aex/dev/tree/driver.hpp"

#include "aex/printk.hpp"
#include "aex/string.hpp"

namespace AEX::Dev::Tree {
    Driver::Driver(const char* name) {
        strncpy(this->name, name, sizeof(this->name));
    }

    Driver::~Driver() {}

    bool Driver::check(Device*) {
        printk(PRINTK_WARN "dev: Driver '%s' has not implemented the check method\n", name);
        return false;
    }

    void Driver::bind(Device*) {
        printk(PRINTK_WARN "dev: Driver '%s' has not implemented the bind method\n", name);
    }

    void register_base_drivers() {}
}