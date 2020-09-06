#include "sys/pit.hpp"

#include "aex/arch/sys/cpu.hpp"

namespace AEX::Sys {
    Spinlock PIT::m_lock;

    void PIT::setHz(int hz) {
        ScopeSpinlock scopeLock(m_lock);

        int divisor = 1193181.66666666 / hz;

        CPU::outportb(0x43, 0x36);
        CPU::outportb(0x40, divisor & 0xFF);
        CPU::outportb(0x40, divisor >> 8);
    }

    void PIT::setInterval(double ms) {
        setHz(1000.0 / ms);
    }

    void PIT::interruptIn(double ms) {
        ScopeSpinlock scopeLock(m_lock);

        double hz      = 1000.0 / ms;
        int    divisor = 1193181.66666666 / hz;

        CPU::outportb(0x43, 0x30);
        CPU::outportb(0x40, divisor & 0xFF);
        CPU::outportb(0x40, divisor >> 8);
    }
}