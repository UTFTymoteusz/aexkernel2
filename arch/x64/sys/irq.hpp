#pragma once

#include <stdint.h>

namespace AEX::Sys::IRQ {
    extern bool is_APIC_present;

    void init();

    void set_vector(int irq, uint8_t vector);

    void set_mask(int irq, bool mask);

    void set_destination(int irq, uint8_t destination);

    void setup_timer();
}