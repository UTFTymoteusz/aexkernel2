#pragma once

#include "aex/errno.hpp"
#include "aex/utility.hpp"

namespace AEX::Sys::Power {
    /**
     * Powers off the system. Handlers get executed in order from the lowest order to the highest
     * order.
     * @returns Doesn't return on success, but returns an error if the poweroff failed.
     **/
    API error_t poweroff();

    API void register_poweroff_handler(int order, error_t (*func)());
    API void unregister_poweroff_handler(error_t (*func)());
}
