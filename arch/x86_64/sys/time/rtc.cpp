#include "sys/time/rtc.hpp"

#include "aex/byte.hpp"
#include "aex/mem.hpp"
#include "aex/sys/acpi.hpp"
#include "aex/sys/acpi/fadt.hpp"
#include "aex/sys/irq.hpp"
#include "aex/sys/time.hpp"

#include "sys/cmos.hpp"

namespace AEX::Sys::Time {
    Spinlock RTC::m_lock;

    bool    RTC::m_normal_hour_format;
    bool    RTC::m_retarded_bcd;
    uint8_t RTC::m_century_index;

    volatile int64_t RTC::m_epoch = 0;

    void RTC::init() {
        SCOPE(m_lock);

        CMOS::write(CMOS::STATUS_B, CMOS::read(CMOS::STATUS_B) | CMOS::STATUS_B_UPDATE_ENDED_INT);
        CMOS::read(CMOS::STATUS_C);

        uint8_t status_b = CMOS::read(CMOS::STATUS_B);

        m_normal_hour_format = status_b & CMOS::STATUS_B_24_HOUR_MODE;
        m_retarded_bcd       = !(status_b & CMOS::STATUS_B_BINARY_MODE);

        IRQ::register_handler(8, RTC::irq);

        auto _fadt = (ACPI::fadt*) ACPI::find_table("FACP", 0);

        RTC::m_century_index = _fadt->century;
        if (RTC::m_century_index == 0)
            m_century_index = 0x32;
    }

    time_t RTC::epoch() {
        time_t ret = Mem::atomic_read(&m_epoch);
        while (!ret)
            ret = Mem::atomic_read(&m_epoch);

        return ret;
    }

    void RTC::irq(void*) {
        CMOS::read(CMOS::STATUS_C);

        uint8_t hours   = CMOS::read(0x04);
        uint8_t minutes = CMOS::read(0x02);
        uint8_t seconds = CMOS::read(0x00);
        uint8_t year    = CMOS::read(0x09);
        uint8_t month   = CMOS::read(0x08);
        uint8_t day     = CMOS::read(0x07);
        uint8_t century = CMOS::read(m_century_index);

        bool pm = (hours & 0x80);

        hours &= ~0x80;

        if (m_retarded_bcd) {
            hours   = fromBCD(hours);
            minutes = fromBCD(minutes);
            seconds = fromBCD(seconds);
            year    = fromBCD(year);
            month   = fromBCD(month);
            day     = fromBCD(day);
            century = fromBCD(century);
        }

        uint32_t actual_year = year + century * 100;

        if (!m_normal_hour_format && pm)
            hours += 12;

        m_epoch = Time::dt2epoch(actual_year, month, day, hours, minutes, seconds);
    }
}