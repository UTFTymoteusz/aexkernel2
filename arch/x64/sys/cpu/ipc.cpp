#include "aex/arch/sys/cpu.hpp"
#include "aex/kpanic.hpp"
#include "aex/mutex.hpp"
#include "aex/printk.hpp"

#include "sys/irq/apic.hpp"
#include "sys/mcore.hpp"

namespace AEX::Sys {
    struct invm_data {
        size_t   addr;
        uint32_t pages;
    };

    extern "C" void proc_reshed();

    Spinlock ipp_lock;

    void CPU::broadcast(ipp_type type, void* data, bool ignore_self) {
        ScopeSpinlock scopeLock(ipp_lock);

        for (int i = 0; i < MCore::cpu_count; i++) {
            if (i == CPU::currentID() && ignore_self)
                continue;

            auto cpu = MCore::CPUs[i];
            if (cpu == nullptr)
                continue;

            cpu->_send(type, data);
        }
    }

    void CPU::send(ipp_type type, void* data) {
        ScopeSpinlock scopeLock(ipp_lock);
        _send(type, data);
    }

    void CPU::_send(ipp_type type, void* data) {
        m_ipi_lock.acquire();
        m_ipi_ack = false;

        m_ipi_packet.type = type;
        m_ipi_packet.data = data;

        if (CPU::currentID() == this->id) {
            handleIPP();
            m_ipi_lock.release();

            return;
        }

        IRQ::APIC::interrupt(apic_id, 32 + 13);

        volatile size_t counter = 0;

        while (!m_ipi_ack) {
            counter++;

            if (counter == 4000000 * 8)
                IRQ::APIC::nmi(apic_id);

            if (counter == 4000000 * 128)
                kpanic("ipi to cpu%i from cpu%i stuck (%i, 0x%p)", this->id, CPU::currentID(), type,
                       data);
        }

        m_ipi_lock.release();
    }

    /**
     * This is a cute little function that's gonna get called by the IPI IRQ handler.
     **/
    extern "C" void ipi_handle() {
        auto us = CPU::current();

        us->in_interrupt++;
        us->handleIPP();
        us->in_interrupt--;

        __sync_synchronize();
    }

    void CPU::handleIPP() {
        auto us = CPU::current();

        switch (m_ipi_packet.type) {
        case IPP_HALT:
            m_ipi_ack = true;

            IRQ::APIC::eoi();
            CPU::halt();

            return;
        case IPP_RESHED:
            m_ipi_ack = true;

            IRQ::APIC::eoi();

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
        case IPP_PG_INVM: {
            auto bong = (invm_data*) m_ipi_packet.data;

            size_t   addr  = bong->addr;
            uint32_t pages = bong->pages + 1; // TODO: Figure out why + 1 makes it work

            for (uint32_t i = 0; i < pages; i++) {
                asm volatile("invlpg [%0]" : : "r"(addr));
                addr += CPU::PAGE_SIZE;
            }

            m_ipi_ack = true;
        } break;
        default:
            m_ipi_ack = true;
            printk(PRINTK_WARN "cpu%i: Received an IPP with an unknown type (%i)\n",
                   CPU::currentID(), m_ipi_packet.type);

            break;
        }

        IRQ::APIC::eoi();
    }
}