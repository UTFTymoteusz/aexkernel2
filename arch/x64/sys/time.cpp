#include "aex/sys/time.hpp"

#include "aex/sys/irq.hpp"

#include "sys/irq.hpp"
#include "sys/rtc.hpp"
#include "sys/time.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    uint64_t get_uptime() {
        return IRQ::get_uptime();
    }

    int64_t get_clock_time() {
        return RTC::get_epoch();
    }

    void init_time() {
        RTC::init();
    }
}