#include "sys/irq/pic.hpp"

#include "aex/arch/sys/cpu.hpp"

constexpr auto ICW1_INIT = 0x10;
constexpr auto ICW1_ICW4 = 0x01;

constexpr auto ICW4_8086 = 0x01;

namespace AEX::Sys::IRQ {
    PIC pics[2];

    void PIC::mask(uint8_t mask) {
        CPU::outportb(data, mask);
    }

    void PIC::init(uint8_t start, bool slave) {
        CPU::outportb(command, ICW1_INIT | ICW1_ICW4);
        CPU::outportb(data, start);
        CPU::outportb(data, slave ? 0b0010 : 0b0100);
        CPU::outportb(data, ICW4_8086);
    }
}