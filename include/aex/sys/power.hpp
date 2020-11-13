#pragma once

#include "aex/errno.hpp"

namespace AEX::Sys::Power {
    error_t poweroff();

    void register_poweroff_handler(int priority, error_t (*func)());
    void unregister_poweroff_handler(error_t (*func)());
}
