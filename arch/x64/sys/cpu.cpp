#include "aex/arch/sys/cpu.hpp"

#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "cpu/idt.hpp"
#include "sys/apic.hpp"

// For some reason g++ adds 8 to the offset
#define CURRENT_CPU                                            \
    ((AEX::Sys::CPU*) ({                                       \
        size_t ret = 0;                                        \
        asm volatile("mov %0, qword [gs:-0x08];" : "=r"(ret)); \
        ret;                                                   \
    }))

constexpr auto GSBase = 0xC0000101;

constexpr auto CPUID_NAME_STRING_1 = 0x80000002;
constexpr auto CPUID_NAME_STRING_2 = 0x80000003;
constexpr auto CPUID_NAME_STRING_3 = 0x80000004;

constexpr auto MSR_PAT = 0x0277;

constexpr auto PAT_COMBINE = 0x01;

constexpr auto CPUID_FEAT_PAT = (1 << 16);

namespace AEX::Sys {
    CPU::CPU(int id) {
        this->id = id;
        apic_id  = APIC::getID();
        self     = this;

        fillAndCleanName();
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

        uint32_t idc, edx;

        cpuid(0x0001, &idc, &idc, &idc, &edx);

        if (!(edx & CPUID_FEAT_PAT)) {
            printk(PRINTK_WARN "cpu%i: PAT not supported\n", id);
            return;
        }

        uint64_t pat = rdmsr(MSR_PAT);

        pat &= ~(7ul << 32);
        pat |= ((uint64_t) PAT_COMBINE << 32);

        wrmsr(MSR_PAT, pat);

        // TSS, wooo
        asm volatile("mov eax, %0; ltr ax;" : : "r"(0x28 + id * 0x10));

        asm volatile("mov rax, cr3; mov cr3, rax;");

        in_interrupt = 1;
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

        return flags & 0x200;
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

    uint64_t CPU::rdmsr(uint32_t reg) {
        uint64_t out;

        asm volatile(" \
        rdmsr; \
        \
        mov %0, rdx; \
        shl %0, 32;  \
        or  %0, rax; \
        "
                     : "=m"(out)
                     : "c"(reg)
                     : "memory");

        return out;
    }


    int CPU::getCurrentCPUID() {
        return CURRENT_CPU->id;
    }

    CPU* CPU::getCurrentCPU() {
        return CURRENT_CPU;
    }

    void CPU::tripleFault() {
        nointerrupts();
        load_idt(nullptr, 0);

        asm volatile("ud2;");
    }

    void CPU::fillAndCleanName() {
        memset(name, '\0', sizeof(name));

        char* ptr = name;

        for (int i = 0; i < 3; i++) {
            cpuid(CPUID_NAME_STRING_1 + i, (uint32_t*) (ptr + 0), (uint32_t*) (ptr + 4),
                  (uint32_t*) (ptr + 8), (uint32_t*) (ptr + 12));

            ptr += 16;
        }

        // Now let's get rid of Intel's annoying right justification
        while (name[0] == ' ') {
            memcpy(name, name + 1, 47);
            name[47] = '\0';
        }
    }
}