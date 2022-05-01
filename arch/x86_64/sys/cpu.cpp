#include "aex/arch/sys/cpu.hpp"

#include "aex/arch/sys/cpu/idt.hpp"
#include "aex/arch/sys/cpu/tss.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/sec/random.hpp"
#include "aex/string.hpp"

#include "proc/proc.hpp"
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

#define PREVIOUS_THREAD                                               \
    ((AEX::Proc::Thread*) ({                                          \
        size_t ret = 0;                                               \
        asm volatile("mov %0, qword [gs:-0x08 + 0x18];" : "=r"(ret)); \
        ret;                                                          \
    }))

constexpr auto MSR_FSBase       = 0xC0000100;
constexpr auto MSR_GSBase       = 0xC0000101;
constexpr auto MSR_KernelGSBase = 0xC0000102;

constexpr auto CPUID_VENDOR_ID     = 0x00000000;
constexpr auto CPUID_NAME_STRING_1 = 0x80000002;
constexpr auto CPUID_NAME_STRING_2 = 0x80000003;
constexpr auto CPUID_NAME_STRING_3 = 0x80000004;

constexpr auto MSR_PAT = 0x0277;

constexpr auto PAT_COMBINE = 0x01;

constexpr auto CPUID_FEAT_PAT = (1 << 16);

namespace AEX::Sys {
    InterruptionGuard<true>  CPU::interruptsGuard;
    InterruptionGuard<false> CPU::nointerruptsGuard;

    CPU::CPU(int id) {
        this->id = id;

        apic_id = IRQ::APIC::id();
        self    = this;
    }

    void CPU::initLocal() {
        setup_idt();
        load_idt(idt, 256);

        asm volatile("    \
            xor rax, rax; \
            mov gs , rax; \
            mov fs , rax; \
        ");

        wrmsr(MSR_GSBase, (size_t) & (this->self));

        uint64_t pat = rdmsr(MSR_PAT);

        pat &= ~(7ul << 32);
        pat |= ((uint64_t) PAT_COMBINE << 32);

        wrmsr(MSR_PAT, pat);

        asm volatile("mov eax, %0; ltr ax;" : : "r"(0x28 + id * 0x10));
        asm volatile("mov rax, cr3; mov cr3, rax;");

        // reshed_fault_stack_start = (size_t) Mem::kernel_pagemap->alloc(4096);
        in_interrupt = 1;

        getVendor();
        getName();
    }

    void CPU::halt() {
        printk(WARN "cpu%i: Halted\n", CPU::currentID());
        Debug::stack_trace();

        CPU::current()->halted = true;

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

        asm volatile("pushfq ; \
                    pop %0;"
                     : "=r"(flags)
                     :);

        return flags & 0x0200;
    }

    void CPU::wait() {
        ASSERT(CPU::checkInterrupts());
        asm volatile("hlt");
    }

    void CPU::cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
        asm volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(code)
                     : "memory");
    }

    uint8_t CPU::inb(uint16_t m_port) {
        uint8_t val;
        asm volatile("inb %0, %1" : "=a"(val) : "dN"(m_port));
        return val;
    }

    uint16_t CPU::inw(uint16_t m_port) {
        uint16_t val;
        asm volatile("inw %0, %1" : "=a"(val) : "dN"(m_port));
        return val;
    }

    uint32_t CPU::ind(uint16_t m_port) {
        uint32_t val;
        asm volatile("ind %0, %1" : "=a"(val) : "d"(m_port));
        return val;
    }

    void CPU::outb(uint16_t m_port, uint8_t m_data) {
        asm volatile("outb %0, %1" : : "dN"(m_port), "a"(m_data));
    }

    void CPU::outw(uint16_t m_port, uint16_t m_data) {
        asm volatile("outw %0, %1" : : "dN"(m_port), "a"(m_data));
    }

    void CPU::outd(uint16_t m_port, uint32_t m_data) {
        asm volatile("outd %0, %1" : : "d"(m_port), "a"(m_data));
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

    void CPU::wrmsr(uint32_t reg, void* data) {
        wrmsr(reg, (uint64_t) data);
    }

    CPU* CPU::current() {
        return CURRENT_CPU;
    }

    int CPU::currentID() {
        return CURRENT_CPU->id;
    }

    Proc::Thread* CPU::currentThread() {
        return CURRENT_THREAD;
    }

    Proc::Thread* CPU::previousThread() {
        return PREVIOUS_THREAD;
    }

    void CPU::tripleFault() {
        printk("cpu%i: Triple fault!\n", currentID());

        nointerrupts();
        load_idt(nullptr, 0);

        asm volatile("ud2;");
        asm volatile("int 2;");
        asm volatile("int 1;");
        asm volatile("int 3;");
        asm volatile("int 7;");
    }

    void CPU::breakpoint(int index, size_t addr, uint8_t trigger, uint8_t size, bool enabled) {
        switch (index) {
        case 0:
            asm volatile("mov dr0, rsi");
            break;
        case 1:
            asm volatile("mov dr1, rsi");
            break;
        case 2:
            asm volatile("mov dr2, rsi");
            break;
        case 3:
            asm volatile("mov dr3, rsi");
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

    void CPU::swthread(Proc::Thread* thread) {
        asm volatile("mov %0, cr3;" : : "r"(thread->context->cr3));

        // 3 weeks of rest because of the goddamned + thread->fault_stack_size
        local_tss->ist1 = (size_t) thread->fault_stack.ptr + thread->fault_stack.size;
        local_tss->ist4 = (size_t) thread->context + 22 * 8;
        local_tss->ist7 = (size_t) thread->kernel_stack.ptr + thread->kernel_stack.size;

        if (thread->faulting)
            local_tss->ist1 -= 8192;

        wrmsr(MSR_FSBase, (size_t) thread->tls);

        if (Proc::ready)
            ASSERT(thread->parent);

        Sec::feed_random(thread->context->rsp);
        Sec::feed_random(IRQ::APIC::counter());
    }

    void CPU::pushFmsg(const char* msg) {
        if (m_fmsg_ptr >= 16)
            return;

        memcpy(m_fmsgs[m_fmsg_ptr++], msg, 256);
    }

    void CPU::printFmsgs() {
        for (int i = 0; i < m_fmsg_ptr; i++)
            printk("  cpu%i: %s\n", id, m_fmsgs[i]);
    }

    int CPU::countFmsgs() {
        return m_fmsg_ptr;
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

    void CPU::getVendor() {
        uint32_t ignore;

        memset(vendor, '\0', sizeof(vendor));
        cpuid(CPUID_VENDOR_ID, &ignore, (uint32_t*) (vendor + 0), (uint32_t*) (vendor + 8),
              (uint32_t*) (vendor + 4));
    }
}