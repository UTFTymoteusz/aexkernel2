#include "sys/cpu.hpp"

#include "aex/printk.hpp"

#include "cpu/idt.hpp"
#include "sys/apic.hpp"

#include <stddef.h>

// For some reason g++ adds 8 to the offset
#define CURRENT_CPU                                            \
    ((AEX::Sys::CPU*) ({                                       \
        size_t ret = 0;                                        \
        asm volatile("mov %0, qword [gs:-0x08];" : "=r"(ret)); \
        ret;                                                   \
    }))

#define GSBase 0xC0000101

namespace AEX::Sys {
    CPU::CPU(int id) {
        this->id = id;
        apic_id  = APIC::getID();
        self     = this;
    }

    void CPU::initLocal() {
        setup_idt();
        load_idt(init_IDT, 256);

        asm volatile("    \
            xor rax, rax; \
            mov gs , rax; \
            mov fs , rax; \
        ");

        wrmsr(GSBase, (size_t) & (this->self));
    }

    void CPU::halt() {
        printk(PRINTK_WARN "cpu%i: Halted\n", CPU::getCurrentCPUID());

        asm volatile("cli;");

        while (true)
            asm volatile("hlt;");
    }

    void CPU::interrupts() {
        asm volatile("sti");
    }

    void CPU::nointerrupts() {
        asm volatile("cli");
    }

    bool CPU::checkInterrupts() {
        size_t flags;

        asm volatile("pushf ; \
                    pop %0;"
                     : "=r"(flags)
                     :);

        return (flags & 0x200) > 0;
    }

    void CPU::waitForInterrupt() {
        asm volatile("hlt");
    }

    void CPU::cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
        asm volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(code)
                     : "memory");
    }

    uint8_t CPU::inportb(uint16_t _port) {
        uint8_t val;
        asm volatile("inb %0, %1" : "=a"(val) : "dN"(_port));
        return val;
    }

    void CPU::outportb(uint16_t _port, uint8_t _data) {
        asm volatile("outb %0, %1" : : "dN"(_port), "a"(_data));
    }

    uint16_t CPU::inportw(uint16_t _port) {
        uint16_t val;
        asm volatile("inw %0, %1" : "=a"(val) : "dN"(_port));
        return val;
    }

    void CPU::outportw(uint16_t _port, uint16_t _data) {
        asm volatile("outw %0, %1" : : "dN"(_port), "a"(_data));
    }

    uint32_t CPU::inportd(uint16_t _port) {
        uint32_t val;
        asm volatile("ind %0, %1" : "=a"(val) : "d"(_port));
        return val;
    }

    void CPU::outportd(uint16_t _port, uint32_t _data) {
        asm volatile("outd %0, %1" : : "d"(_port), "a"(_data));
    }

    void CPU::wrmsr(uint32_t reg, uint64_t data) {
        asm volatile(" \
        mov rdx, %0; \
        mov rax, %0; \
        \
        ror rdx, 32; \
        \
        wrmsr; \
        "
                     :
                     : "r"(data), "c"(reg)
                     : "memory");
    }


    int CPU::getCurrentCPUID() {
        return CURRENT_CPU->id;
    }

    CPU* CPU::getCurrentCPU() {
        return CURRENT_CPU;
    }
}