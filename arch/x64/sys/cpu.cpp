#include "sys/cpu.hpp"

#include "cpu/idt.hpp"
#include "kernel/kpanic.hpp"
#include "kernel/printk.hpp"
#include "kernel/string.hpp"
#include "mem/pmem.hpp"
#include "sys/apic.hpp"
#include "sys/mcore.hpp"
#include "sys/pic.hpp"

#define CPUID_EDX_FEAT_APIC 0x100

#define GSBase 0xC0000101

namespace AEX::Sys {
    int  CPU::PAGE_SIZE     = 4096;
    bool CPU::isAPICPresent = false;

    bool ioapicReady = false;
    bool idtReady    = false;

    extern "C" char exc_array;
    extern "C" char irq_array;

    extern "C" char irq_spurious;

    IDTEntry initIDT[256];

    CPU::CPU(int id) {
        this->id = id;
        self     = this;

        CPU::localInit(id, !idtReady);
        idtReady = true;
    }

    void CPU::globalInit() {
        pics[0] = PIC(0x20, 0x21);
        pics[1] = PIC(0xA0, 0xA1);
    }

    void CPU::halt() {
        printk(PRINTK_WARN "cpu%i: Halted\n", CPU::getCurrentCPUID());

        asm volatile("cli;");

        while (true)
            asm volatile("hlt;");
    }

    void CPU::interrupts() { asm volatile("sti"); }

    void CPU::nointerrupts() { asm volatile("cli"); }

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

    int CPU::getCurrentCPUID() { return CURRENT_CPU->id; }

    CPU* CPU::getCurrentCPU() { return CURRENT_CPU; }

    void CPU::sendPacket(uint32_t type, void* data) {
        _ipi_lock.acquire();
        _ipi_ack = false;

        _ipi_packet.type = type;
        _ipi_packet.data = data;

        APIC::sendInterrupt(apic_id, 32 + 13);

        while (!_ipi_ack)
            ;

        _ipi_lock.release();
    }

    void CPU::broadcastPacket(uint32_t type, void* data, bool ignore_self) {
        for (int i = 0; i < MCore::CPUs.count(); i++) {
            if (i == CPU::getCurrentCPUID() && ignore_self)
                continue;

            auto cpu = MCore::CPUs.get(i);
            if (!cpu.isPresent())
                continue;

            cpu->sendPacket(type, data);
        }
    }

    void CPU::setup_idt() {
        memset(initIDT, 0, sizeof(initIDT));

        size_t* _exc_array = (size_t*) &exc_array;

        for (int i = 0; i < 32; i++) {
            initIDT[i].setOffset(_exc_array[i]);
            initIDT[i].setSelector(0x08);
            initIDT[i].setType(0x0E);
            initIDT[i].setPresent(true);
        }

        size_t* _irq_array = (size_t*) &irq_array;

        for (int i = 0; i < 32; i++) {
            initIDT[i + 32].setOffset(_irq_array[i]);
            initIDT[i + 32].setSelector(0x08);
            initIDT[i + 32].setType(0x0E);
            initIDT[i + 32].setPresent(true);
        }

        size_t spurious = (size_t) &irq_spurious;

        initIDT[255].setOffset(spurious);
        initIDT[255].setSelector(0x08);
        initIDT[255].setType(0x0E);
        initIDT[255].setPresent(true);
    }

    void CPU::setup_irq() {
        uint32_t eax, ebx, ecx, edx;

        cpuid(0x01, &eax, &ebx, &ecx, &edx);
        isAPICPresent = edx & CPUID_EDX_FEAT_APIC;

        if (isAPICPresent) {
            MASTER_PIC->init(32, false);
            MASTER_PIC->setMask(0b11111111);

            SLAVE_PIC->init(40, true);
            SLAVE_PIC->setMask(0b11111111);

            // alloc the physical page taken by the APIC
            APIC::map(0xFEE00000);
            APIC::write(0xF0, APIC::read(0xF0) | 0x1FF);

            apic_id = APIC::read(0x20) >> 24;
            APIC::write(0x20, (apic_id << 24) | 1488);

            if (!ioapicReady) {
                IOAPIC::map(0xFEC00000);

                for (int i = 0; i < 24; i++)
                    IOAPIC::entries[i].setVector(32 + i);

                ioapicReady = true;
            }
        }
        else {
            // printk("We have a PIC\n");
        }
    }

    void CPU::localInit(int id, bool idt) {
        if (idt) {
            setup_idt();
            load_idt(initIDT, 256);
        }

        setup_irq();

        asm volatile("    \
            xor rax, rax; \
            mov gs , rax; \
            mov fs , rax; \
        ");

        wrmsr(GSBase, (size_t) & (this->self));
    }

    /*
     * This is a little cute function that's gonna get called by our IRQ handler.
     */
    extern "C" void ipi_handle() {
        auto us = CPU::getCurrentCPU();

        us->handle_ipp();
    }

    void CPU::handle_ipp() {
        switch (_ipi_packet.type) {
        case CPU::ipp_type::HALT:
            _ipi_ack = true; // We have to ack here, we may get cause an infinite loop otherwise
            CPU::halt();

            break;
        default:
            printk(PRINTK_WARN "cpu%i: Received an unknown IPP type (%i)\n", CPU::getCurrentCPUID(),
                   _ipi_packet.type);

            break;
        }

        _ipi_ack = true;
    }
}