#pragma once

#include <stdint.h>

namespace AEX::Proc {
    struct resource_usage {
        uint64_t cpu_time_ns;

        uint64_t block_bytes_read;
        uint64_t block_bytes_written;
    };
}
