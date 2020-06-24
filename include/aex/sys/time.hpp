#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    struct date_time {
        uint32_t year;
        uint8_t  month;
        uint8_t  day;

        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    /**
     * Gets the amount of time passed since multicore timers were setup.
     * @returns Amount of time passed in nanoseconds.
     */
    uint64_t get_uptime();

    /**
     * Returns the clock time as an UNIX epoch.
     */
    int64_t get_clock_time();

    int64_t to_unix_epoch(uint32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute,
                          uint8_t second);

    inline int64_t to_unix_epoch(date_time dt) {
        return to_unix_epoch(dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    }

    date_time from_unix_epoch(int64_t epoch);
}