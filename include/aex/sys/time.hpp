#pragma once

#include "aex/sys/time/types.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::Time {
    /**
     * Gets the amount of time passed since AEX has booted.
     * @returns Amount of time passed in nanoseconds.
     */
    time_t uptime();

    /**
     * Returns the clock time as an UNIX epoch.
     */
    time_t clocktime();

    time_t dt2epoch(uint32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute,
                    uint8_t second);

    inline time_t dt2epoch(date_time dt) {
        return dt2epoch(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    }

    date_time epoch2dt(time_t epoch);
}