#include "sys/irq/pit.hpp"

#include "aex/arch/sys/cpu.hpp"

namespace AEX::Sys::IRQ {
    Spinlock PIT::m_lock;

    void PIT::hz(int hz) {
        SCOPE(m_lock);

        int divisor = 1193181.66666666 / hz;

        CPU::outb(0x43, 0x36);
        CPU::outb(0x40, divisor & 0xFF);
        CPU::outb(0x40, divisor >> 8);
    }

    void PIT::interval(double ms) {
        hz(1000.0 / ms);
    }

    void PIT::interrupt(double ms) {
        SCOPE(m_lock);

        double hz      = 1000.0 / ms;
        int    divisor = 1193181.66666666 / hz;

        CPU::outb(0x43, 0x30);
        CPU::outb(0x40, divisor & 0xFF);
        CPU::outb(0x40, divisor >> 8);
    }
}