#pragma once

#include <stdint.h>

namespace AEX::Sys::Time {
    typedef int64_t time_t;

    void init();

    time_t uptime_raw();

    void lazy_sleep(uint64_t ms);
}