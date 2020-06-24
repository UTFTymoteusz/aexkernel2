#include "aex/sys/time.hpp"

namespace AEX::Sys {
    uint8_t day_count_for_each_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31};

    int64_t to_unix_epoch(uint32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute,
                          uint8_t second) {
        uint32_t year_calc      = year - 1900;
        uint16_t day_since_jan1 = 0;

        for (int i = 0; i < month - 1; i++)
            day_since_jan1 += day_count_for_each_month[i];

        day_since_jan1 += day;

        return second + minute * 60 + hour * 3600 + day_since_jan1 * 86400 +
               (year_calc - 70) * 31536000 + ((year_calc - 69) / 4) * 86400 -
               ((year_calc - 1) / 100) * 86400 + ((year_calc + 299) / 400) * 86400;
    }

    // gonna need to make this handle leaps properly
    date_time from_unix_epoch(int64_t epoch) {
        auto dt = date_time();

        dt.year = 1970 + (epoch / 31536000);

        uint32_t year_calc = dt.year - 1900;

        epoch -= (year_calc - 70) * 31536000;
        epoch -= ((year_calc - 69) / 4) * 86400;
        epoch += ((year_calc - 1) / 100) * 86400;
        epoch -= ((year_calc + 299) / 400) * 86400;

        dt.second = epoch % 60;
        dt.minute = (epoch / 60) % 60;
        dt.hour   = (epoch / 3600) % 24;

        uint16_t day = epoch / 86400;

        for (int i = 0; i < 12; i++) {
            if (day < day_count_for_each_month[i]) {
                dt.month = i + 1;
                break;
            }

            day -= day_count_for_each_month[i];
        }

        dt.day = day;

        return dt;
    }
}