#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/kpanic.hpp"
#include "aex/mutex.hpp"
#include "aex/printk.hpp"

#include "proc/proc.hpp"
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
        SCOPE(ipp_lock);

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
        SCOPE(ipp_lock);
        _send(type, data);
    }

    void CPU::_send(ipp_type type, void* data) {
        m_ipi_lock.acquire();

        m_ipi_packet.type = type;
        m_ipi_packet.data = data;
        m_ipi_packet.ack  = false;

        if (CPU::currentID() == this->id) {
            interruptible(false) {
                handleIPP();
            }

            m_ipi_lock.release();
            return;
        }

        IRQ::APIC::interrupt(apic_id, 32 + 13);

        volatile size_t counter = 0;

        while (!m_ipi_packet.ack) {
            counter++;

            AEX_ASSERT(Sys::CPU::current()->id != this->id);

            if (counter == 100000000l * 4) {
                IRQ::APIC::nmi(apic_id);
            }

            if (counter == 100000000l * 8) {
                m_ipi_lock.release();
                kpanic("ipi to cpu%i from cpu%i stuck (%i, 0x%p)", this->id, CPU::currentID(), type,
                       data);
            }
        }

        m_ipi_lock.release();

        if (halted)
            CPU::halt();
    }

    /**
     * This is a cute little function that's gonna get called by the IPI IRQ handler.
     **/
    extern "C" void ipi_handle() {
        auto us = CPU::current();

        us->in_interrupt++;
        us->handleIPP();
        us->in_interrupt--;
    }

    void CPU::handleIPP() {
        auto us = CPU::current();

        AEX_ASSERT(us->m_ipi_lock.isAcquired());
        AEX_ASSERT(!CPU::checkInterrupts());

        switch (m_ipi_packet.type) {
        case IPP_HALT:
            m_ipi_packet.ack = true;

            if (us->in_interrupt)
                IRQ::APIC::eoi();

            CPU::halt();

            return;
        case IPP_RESHED:
            m_ipi_packet.ack = true;

            if (us->in_interrupt)
                IRQ::APIC::eoi();

            us->in_interrupt--;
            proc_reshed();
            us->in_interrupt++;

            __sync_synchronize();
            return;
        case IPP_CALL:
            ((void (*)(void)) m_ipi_packet.data)();
            m_ipi_packet.ack = true;
            break;
        case IPP_PG_FLUSH:
            flushPg();
            m_ipi_packet.ack = true;

            break;
        case IPP_PG_INV:
            flushPg(m_ipi_packet.data);
            m_ipi_packet.ack = true;

            break;
        case IPP_PG_INVM: {
            auto bong = (invm_data*) m_ipi_packet.data;

            size_t   addr  = bong->addr;
            uint32_t pages = bong->pages;

            if (us->in_interrupt)
                IRQ::APIC::eoi();

            for (uint32_t i = 0; i < pages; i++) {
                flushPg(addr);
                addr += CPU::PAGE_SIZE;
            }

            m_ipi_packet.ack = true;
        }
            return;
        default:
            m_ipi_packet.ack = true;
            printk(PRINTK_WARN "cpu%i: Received an IPP with an unknown type (%i)\n",
                   CPU::currentID(), m_ipi_packet.type);

            break;
        }

        if (us->in_interrupt)
            IRQ::APIC::eoi();

        __sync_synchronize();
    }
}