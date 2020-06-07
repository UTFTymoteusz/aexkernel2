#include "aex/sys/time.hpp"

#include "sys/irq.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    uint64_t get_uptime() {
        return IRQ::get_uptime();
    }
}