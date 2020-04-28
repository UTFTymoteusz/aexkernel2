#include "sys/pic.hpp"

#include "sys/cpu.hpp"

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01

#define ICW4_8086 0x01

namespace AEX::Sys {
    PIC pics[2];

    void PIC::setMask(uint8_t mask) {
        CPU::outportb(data, mask);
    }

    void PIC::init(uint8_t start, bool slave) {
        CPU::outportb(command, ICW1_INIT | ICW1_ICW4);
        CPU::outportb(data, start);
        CPU::outportb(data, slave ? 0b0010 : 0b0100);
        CPU::outportb(data, ICW4_8086);
    }
}