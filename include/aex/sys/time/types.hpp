#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::Time {
    struct date_time {
        uint32_t year;
        uint8_t  month;
        uint8_t  day;

        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    typedef int64_t time_t;
    typedef int64_t timediff_t;
}