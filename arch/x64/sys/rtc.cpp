#include "sys/rtc.hpp"

#include "aex/byte.hpp"
#include "aex/mem/atomic.hpp"
#include "aex/printk.hpp"
#include "aex/sys/irq.hpp"
#include "aex/sys/time.hpp"

#include "sys/cmos.hpp"

namespace AEX::Sys {
    Spinlock RTC::_lock;

    bool RTC::_normal_hour_format;
    bool RTC::_retarded_bcd;

    volatile int64_t RTC::_epoch;

    void RTC::init() {
        auto scopeLock = ScopeSpinlock(_lock);

        CMOS::write(CMOS::STATUS_B, CMOS::read(CMOS::STATUS_B) | CMOS::STATUS_B_UPDATE_ENDED_INT);
        CMOS::read(CMOS::STATUS_C);

        uint8_t status_b = CMOS::read(CMOS::STATUS_B);

        _normal_hour_format = status_b & CMOS::STATUS_B_24_HOUR_MODE;
        _retarded_bcd       = !(status_b & CMOS::STATUS_B_BINARY_MODE);

        IRQ::register_handler(8, RTC::irq);
    }

    int64_t RTC::get_epoch() {
        return Mem::atomic_read(&_epoch);
    }

    void RTC::irq(void*) {
        auto scopeLock = ScopeSpinlock(_lock);

        CMOS::read(CMOS::STATUS_C);

        uint8_t hours   = CMOS::read(0x04);
        uint8_t minutes = CMOS::read(0x02);
        uint8_t seconds = CMOS::read(0x00);

        uint8_t year  = CMOS::read(0x09);
        uint8_t month = CMOS::read(0x08);
        uint8_t day   = CMOS::read(0x07);

        // prob should check the ACPI tables
        uint8_t century = CMOS::read(0x32);

        bool pm = (hours & 0x80);

        hours &= ~0x80;

        if (_retarded_bcd) {
            hours   = fromBCD(hours);
            minutes = fromBCD(minutes);
            seconds = fromBCD(seconds);

            year  = fromBCD(year);
            month = fromBCD(month);
            day   = fromBCD(day);

            century = fromBCD(century);
        }

        uint32_t actual_year = year + century * 100;

        if (!_normal_hour_format && pm)
            hours += 12;

        _epoch = to_unix_epoch(actual_year, month, day, hours, minutes, seconds);

        /*printk("a %02i:%02i:%02i %02i/%02i/%04i (%li)\n", hours, minutes, seconds, day, month,
               actual_year, _epoch);

        auto dt = from_unix_epoch(_epoch);

        printk("b %02i:%02i:%02i %02i/%02i/%04i (%li)\n", dt.hour, dt.minute, dt.second, dt.day,
               dt.month, dt.year, to_unix_epoch(dt));*/
    }
}