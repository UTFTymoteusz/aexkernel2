#include "aex/sys/time.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/kpanic.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/spinlock.hpp"
#include "aex/sys/acpi.hpp"
#include "aex/sys/irq.hpp"

#include "sys/irq.hpp"
#include "sys/pit.hpp"
#include "sys/rtc.hpp"
#include "sys/time.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys {
    constexpr auto ACPI_PM_TIMER_FREQUENCY = 3579545;

    Spinlock uptime_lock = {};

    uint64_t ns_uptime = 0;

    uint64_t acpi_pm_timer_ticks = 0;
    uint32_t acpi_pm_timer_addr  = 0;
    uint32_t acpi_pm_timer_overflow_correction;

    uint64_t get_uptime() {
        auto scopeLock = ScopeSpinlock(uptime_lock);

        uint64_t delta;

        uint64_t ticks = Sys::CPU::inportd(acpi_pm_timer_addr);
        if (ticks < acpi_pm_timer_ticks)
            delta = (ticks + acpi_pm_timer_overflow_correction) - acpi_pm_timer_ticks;
        else
            delta = ticks - acpi_pm_timer_ticks;

        uint64_t ret = Mem::atomic_add_fetch(
            &ns_uptime, (uint64_t)((delta / (double) ACPI_PM_TIMER_FREQUENCY) * 1000000000.0));

        acpi_pm_timer_ticks = ticks;

        return ret;
    }

    int64_t get_clock_time() {
        return RTC::get_epoch();
    }

    void init_time() {
        RTC::init();

        auto _fadt = (ACPI::fadt*) ACPI::find_table("FACP", 0);

        acpi_pm_timer_addr                = _fadt->pm_timer_block;
        acpi_pm_timer_overflow_correction = (_fadt->fixed_flags & (1 << 8)) ? 0xFFFFFFFF : 0xFFFFFF;
    }
}