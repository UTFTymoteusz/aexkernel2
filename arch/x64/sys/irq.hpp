#pragma once

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::IRQ {
    extern bool   is_apic_present;
    extern size_t apic_tps;

    void init();

    /**
     * Sets the vector of an IRQ.
     * @param irq    Absolute IRQ number.
     * @param vector Vector number.
     */
    void set_vector(int irq, uint8_t vector);

    /**
     * Masks an IRQ.
     * @param irq  Absolute IRQ number.
     * @param mask The IRQ mask.
     */
    void set_mask(int irq, bool mask);

    /**
     * Sets the destination of an IRQ.
     * @param irq         Absolute IRQ number.
     * @param destination APIC id of a CPU.
     */
    void set_destination(int irq, uint8_t destination);

    /**
     * Calibrates the timer.
     */
    void init_timer();

    /**
     * Performs a timer IRQ-assisted sleep. This function overwrites the existing interrupt vector
     * so it'll be necessary reconfigure the timer after using this function.
     * @param ms How long to sleep in milliseconds.
     */
    void irq_sleep(double ms);

    void setup_timers_mcore(double hz);

    /**
     * Signalizes a timer tick. This is used for get_uptime. Should not be called directly.
     */
    void timer_tick();

    /**
     * Gets the amount of time passed since multicore timers were setup.
     * @return Amount of time passed in nanoseconds.
     */
    uint64_t get_uptime();
}