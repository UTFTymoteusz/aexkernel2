#include "aex/sys/time.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/sys/acpi.hpp"
#include "aex/sys/acpi/fadt.hpp"
#include "aex/sys/irq.hpp"

#include "sys/irq.hpp"
#include "sys/irq/pit.hpp"
#include "sys/time.hpp"
#include "sys/time/rtc.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::Time {
    constexpr auto ACPI_PM_TIMER_FREQUENCY = 3579545;

    Spinlock uptime_lock;
    time_t   ns_uptime = 0;

    uint64_t acpi_pm_timer_ticks = 0;
    uint32_t acpi_pm_timer_addr  = 0;
    uint32_t acpi_pm_timer_overflow_correction;

    void init() {
        RTC::init();

        auto fadt = (ACPI::fadt*) ACPI::find_table("FACP", 0);

        acpi_pm_timer_addr                = fadt->pm_timer_block;
        acpi_pm_timer_overflow_correction = (fadt->fixed_flags & (1 << 8)) ? 0xFFFFFFFF : 0xFFFFFF;
    }

    time_t uptime() {
        // clang-format off
        Sys::CPU::checkInterrupts() ? uptime_lock.acquire() : uptime_lock.acquireRaw();

        time_t ret;
        uint64_t delta;
        uint64_t ticks = Sys::CPU::ind(acpi_pm_timer_addr);

        if (ticks < acpi_pm_timer_ticks)
            delta = ticks + (acpi_pm_timer_overflow_correction - acpi_pm_timer_ticks);
        else
            delta = ticks - acpi_pm_timer_ticks;

        ret = Mem::atomic_add_fetch(
            &ns_uptime, (time_t) ((delta / (double) ACPI_PM_TIMER_FREQUENCY) * 1000000000.0));

        acpi_pm_timer_ticks = ticks;

        Sys::CPU::checkInterrupts() ? uptime_lock.release() : uptime_lock.releaseRaw();
        // clang-format on

        return ret;
    }

    time_t clocktime() {
        return RTC::epoch();
    }

    void lazy_sleep(uint64_t ms) {
        time_t trigger = Time::uptime() + ms * 1000000;
        while (trigger >= Time::uptime())
            ;
    }
}