#include "aex/arch/sys/cpu.hpp"

#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "cpu/idt.hpp"
#include "cpu/tss.hpp"
#include "sys/irq/apic.hpp"

// For some reason g++ adds 8 to the offset
#define CURRENT_CPU                                            \
    ((AEX::Sys::CPU*) ({                                       \
        size_t ret = 0;                                        \
        asm volatile("mov %0, qword [gs:-0x08];" : "=r"(ret)); \
        ret;                                                   \
    }))

#define CURRENT_THREAD                                                \
    ((AEX::Proc::Thread*) ({                                          \
        size_t ret = 0;                                               \
        asm volatile("mov %0, qword [gs:-0x08 + 0x10];" : "=r"(ret)); \
        ret;                                                          \
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
        apic_id  = IRQ::APIC::getID();
        self     = this;

        getName();
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
        printk(PRINTK_WARN "cpu%i: Halted\n", CPU::getCurrentID());

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

    uint8_t CPU::inportb(uint16_t m_port) {
        uint8_t val;
        asm volatile("inb %0, %1" : "=a"(val) : "dN"(m_port));
        return val;
    }

    void CPU::outportb(uint16_t m_port, uint8_t m_data) {
        asm volatile("outb %0, %1" : : "dN"(m_port), "a"(m_data));
    }

    uint16_t CPU::inportw(uint16_t m_port) {
        uint16_t val;
        asm volatile("inw %0, %1" : "=a"(val) : "dN"(m_port));
        return val;
    }

    void CPU::outportw(uint16_t m_port, uint16_t m_data) {
        asm volatile("outw %0, %1" : : "dN"(m_port), "a"(m_data));
    }

    uint32_t CPU::inportd(uint16_t m_port) {
        uint32_t val;
        asm volatile("ind %0, %1" : "=a"(val) : "d"(m_port));
        return val;
    }

    void CPU::outportd(uint16_t m_port, uint32_t m_data) {
        asm volatile("outd %0, %1" : : "d"(m_port), "a"(m_data));
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


    int CPU::getCurrentID() {
        return CURRENT_CPU->id;
    }

    CPU* CPU::getCurrent() {
        return CURRENT_CPU;
    }

    Proc::Thread* CPU::getCurrentThread() {
        return CURRENT_THREAD;
    }

    void CPU::tripleFault() {
        nointerrupts();
        load_idt(nullptr, 0);

        asm volatile("ud2;");
        asm volatile("int 0;");
    }

    void CPU::setBreakpoint(int index, size_t addr, uint8_t trigger, uint8_t size, bool enabled) {
        switch (index) {
        case 0:
            asm volatile("mov dr0, rsi");
            break;
        default:
            break;
        }

        addr    = addr;
        trigger = trigger;

        size_t dr7;
        asm volatile("mov %0, dr7" : "=r"(dr7));

        if (enabled)
            dr7 |= 0x02 << index * 2;
        else
            dr7 &= ~(0x02 << index * 2);

        dr7 &= ~(0x0F << (18 + index * 4));
        dr7 |= trigger << (16 + index * 4);
        dr7 |= size << (18 + index * 4);

        asm volatile("mov dr7, %0" : : "r"(dr7));
    }

    void CPU::update(Proc::Thread* thread) {
        m_tss->ist1 = thread->fault_stack;
    }

    void CPU::printDebug() {
        printk("ist1: 0x%p\n", m_tss->ist1);
    }

    void CPU::getName() {
        memset(name, '\0', sizeof(name));

        char* ptr = name;

        for (int i = 0; i < 3; i++) {
            cpuid(CPUID_NAME_STRING_1 + i, (uint32_t*) (ptr + 0), (uint32_t*) (ptr + 4),
                  (uint32_t*) (ptr + 8), (uint32_t*) (ptr + 12));

            ptr += 16;
        }

        // Now let's get rid of Intel's annoying right justification
        int pad_count = 0;
        while (name[pad_count] == ' ')
            pad_count++;

        memcpy(name, name + pad_count, 48 - pad_count);
        memset(name + 48 - pad_count, '\0', pad_count);

        name[47] = '\0';
    }
}