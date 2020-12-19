#pragma once

#include "aex/errno.hpp"

namespace AEX::Sys::Power {
    /**
     * Powers off the system. Handlers get executed in order from the lowest order to the highest
     * order.
     * @returns Doesn't return on success, but returns an error if the poweroff failed.
     **/
    error_t poweroff();

    void register_poweroff_handler(int order, error_t (*func)());
    void unregister_poweroff_handler(error_t (*func)());
}
