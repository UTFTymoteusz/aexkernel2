#pragma once

#include "aex/utility.hpp"

#include <stdint.h>

namespace AEX::Proc {
    struct API resource_usage {
        uint64_t cpu_time_ns;

        uint64_t block_bytes_read;
        uint64_t block_bytes_written;
    };
}
