#include "aex/dev/driver.hpp"

#include "aex/string.hpp"

namespace AEX::Dev {
    Driver::Driver(const char* name) {
        strncpy(this->name, name, sizeof(this->name));
    }

    Driver::~Driver() {}

    bool Driver::check(Device* device) {
        return false;
    }

    void Driver::bind(Device* device) {}

    extern void register_sata();

    void register_base_drivers() {
        register_sata();
    }
}