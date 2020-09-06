#include "sys/cmos.hpp"

#include "aex/arch/sys/cpu.hpp"

using CPU = AEX::Sys::CPU;

namespace AEX::Sys {
    Spinlock CMOS::m_lock;

    uint8_t CMOS::read(uint8_t addr) {
        ScopeSpinlock scopeLock(m_lock);

        CPU::outportb(0x70, addr);
        return CPU::inportb(0x71);
    }

    void CMOS::write(uint8_t addr, uint8_t val) {
        ScopeSpinlock scopeLock(m_lock);

        CPU::outportb(0x70, addr);
        CPU::outportb(0x71, val);
    }
}