#pragma once

#include <stdint.h>

namespace AEX::Sys {
    void init_time();

    uint64_t get_uptime_raw();

    void lazy_sleep(uint64_t ms);
}