#include "aex/arch/sys/cpu.hpp"
#include "aex/printk.hpp"

#include "sys/apic.hpp"
#include "sys/mcore.hpp"

namespace AEX::Sys {
    void CPU::broadcastPacket(ipp_type type, void* data, bool ignore_self) {
        for (int i = 0; i < MCore::cpu_count; i++) {
            if (i == CPU::getCurrentCPUID() && ignore_self)
                continue;

            auto cpu = MCore::CPUs[i];
            if (cpu == nullptr)
                continue;

            cpu->sendPacket(type, data);
        }
    }

    void CPU::sendPacket(ipp_type type, void* data) {
        _ipi_lock.acquire();
        _ipi_ack = false;

        _ipi_packet.type = type;
        _ipi_packet.data = data;

        if (CPU::getCurrentCPUID() == this->id) {
            handleIPP();
            _ipi_lock.release();

            return;
        }

        APIC::sendInterrupt(apic_id, 32 + 13);

        while (!_ipi_ack)
            ;

        _ipi_lock.release();
    }

    /**
     * This is a cute little function that's gonna get called by the IPI IRQ handler.
     */
    extern "C" void ipi_handle() {
        auto us = CPU::getCurrentCPU();

        us->handleIPP();
    }

    void CPU::handleIPP() {
        switch (_ipi_packet.type) {
        case CPU::ipp_type::HALT:
            _ipi_ack = true;

            APIC::eoi();
            CPU::halt();

            return;
        case CPU::ipp_type::RESHED:
            _ipi_ack = true;
            printk("cpu%i: Reshed\n", CPU::getCurrentCPUID());

            break;
        case CPU::ipp_type::CALL:
            _ipi_ack = true;
            ((void (*)(void)) _ipi_packet.data)();

            break;
        case CPU::ipp_type::PG_FLUSH:
            _ipi_ack = true;
            asm volatile("mov rax, cr3; mov cr3, rax;");

            break;
        default:
            _ipi_ack = true;
            printk(PRINTK_WARN "cpu%i: Received an IPP with an unknown type (%i)\n",
                   CPU::getCurrentCPUID(), _ipi_packet.type);

            break;
        }

        APIC::eoi();
    }
}