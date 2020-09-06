#include "aex/arch/sys/cpu.hpp"
#include "aex/kpanic.hpp"
#include "aex/printk.hpp"

#include "sys/apic.hpp"
#include "sys/mcore.hpp"

namespace AEX::Sys {
    extern "C" void proc_reshed();

    void CPU::broadcastPacket(ipp_type type, void* data, bool ignore_self) {
        bool ints = CPU::checkInterrupts();

        CPU::nointerrupts();

        for (int i = 0; i < MCore::cpu_count; i++) {
            if (i == CPU::getCurrentID() && ignore_self)
                continue;

            auto cpu = MCore::CPUs[i];
            if (cpu == nullptr)
                continue;

            cpu->sendPacket(type, data);
        }

        if (ints)
            CPU::interrupts();
    }

    void CPU::sendPacket(ipp_type type, void* data) {
        static Spinlock lock;
        ScopeSpinlock   scopeLock(lock);

        m_ipi_lock.acquire();
        m_ipi_ack = false;

        m_ipi_packet.type = type;
        m_ipi_packet.data = data;

        if (CPU::getCurrentID() == this->id) {
            handleIPP();
            m_ipi_lock.release();

            return;
        }

        APIC::sendInterrupt(apic_id, 32 + 13);

        volatile size_t counter = 0;

        while (!m_ipi_ack) {
            counter++;

            if (counter == 4000000 * 7)
                APIC::sendNMI(apic_id);

            if (counter == 4000000 * 8)
                kpanic("ipi to cpu%i from cpu%i stuck (%i, 0x%p)\n", this->id, CPU::getCurrentID(),
                       type, data);
        }

        m_ipi_lock.release();
    }

    /**
     * This is a cute little function that's gonna get called by the IPI IRQ handler.
     */
    extern "C" void ipi_handle() {
        auto us = CPU::getCurrent();

        us->in_interrupt++;
        us->handleIPP();
        us->in_interrupt--;

        __sync_synchronize();
    }

    void CPU::handleIPP() {
        auto us = CPU::getCurrent();

        switch (m_ipi_packet.type) {
        case IPP_HALT:
            m_ipi_ack = true;

            APIC::eoi();
            CPU::halt();

            return;
        case IPP_RESHED:
            m_ipi_ack = true;

            APIC::eoi();

            us->in_interrupt--;
            proc_reshed();
            us->in_interrupt++;

            return;
        case IPP_CALL:
            m_ipi_ack = true;
            ((void (*)(void)) m_ipi_packet.data)();

            break;
        case IPP_PG_FLUSH:
            m_ipi_ack = true;
            asm volatile("mov rax, cr3; mov cr3, rax;");

            break;
        case IPP_PG_INV:
            asm volatile("invlpg [%0]" : : "r"(m_ipi_packet.data));
            m_ipi_ack = true;

            break;
        default:
            m_ipi_ack = true;
            printk(PRINTK_WARN "cpu%i: Received an IPP with an unknown type (%i)\n",
                   CPU::getCurrentID(), m_ipi_packet.type);

            break;
        }

        APIC::eoi();
    }
}