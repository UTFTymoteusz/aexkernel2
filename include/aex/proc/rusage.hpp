#pragma once

#include "aex/sys/time/types.hpp"
#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Proc {
    struct API rusage {
        Sys::Time::clock_t cpu_time_ns;

        uint64_t block_bytes_read;
        uint64_t block_bytes_written;
    };
}
