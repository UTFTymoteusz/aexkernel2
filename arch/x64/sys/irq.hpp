#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::IRQ {
    extern bool   is_APIC_present;
    extern size_t APIC_tps;

    void init();

    void set_vector(int irq, uint8_t vector);
    void set_mask(int irq, bool mask);
    void set_destination(int irq, uint8_t destination);

    void setup_timer(double hz);
    void irq_sleep(double ms);

    void setup_timers_mcore(double hz);
}