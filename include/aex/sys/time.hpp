#pragma once

#include <stdint.h>

namespace AEX::Sys {
    /**
     * Gets the amount of time passed since multicore timers were setup.
     * @returns Amount of time passed in nanoseconds.
     */
    uint64_t get_uptime();
}