#include "sys/pit.hpp"

#include "sys/cpu.hpp"

namespace AEX::Sys {
    void PIT::setHz(int hz) {
        int divisor = 1193181.66666666 / hz;

        CPU::outportb(0x43, 0x36);
        CPU::outportb(0x40, divisor & 0xFF);
        CPU::outportb(0x40, divisor >> 8);
    }

    void PIT::setInterval(double ms) {
        setHz(1000.0 / ms);
    }

    void PIT::interruptIn(double ms) {
        double hz      = 1000.0 / ms;
        int    divisor = 1193181.66666666 / hz;

        CPU::outportb(0x43, 0x30);
        CPU::outportb(0x40, divisor & 0xFF);
        CPU::outportb(0x40, divisor >> 8);
    }
}