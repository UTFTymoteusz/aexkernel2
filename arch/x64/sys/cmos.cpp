#include "sys/cmos.hpp"

#include "aex/arch/sys/cpu.hpp"

using CPU = AEX::Sys::CPU;

namespace AEX::Sys {
    Spinlock CMOS::_lock;

    uint8_t CMOS::read(uint8_t addr) {
        auto scopeLock = ScopeSpinlock(_lock);

        CPU::outportb(0x70, addr);
        return CPU::inportb(0x71);
    }

    void CMOS::write(uint8_t addr, uint8_t val) {
        auto scopeLock = ScopeSpinlock(_lock);

        CPU::outportb(0x70, addr);
        CPU::outportb(0x71, val);
    }
}