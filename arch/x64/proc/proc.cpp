#include "proc/proc.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/printk.hpp"

#include "cpu/idt.hpp"
#include "sys/apic.hpp"
#include "sys/irq.hpp"

namespace AEX::Proc {
    extern "C" void proc_timer_tick();

    void reload() {
        Sys::load_idt(Sys::init_IDT, 256);
    }

    void setup_irq() {
        AEX::Sys::init_IDT[0x20 + 0].setOffset((size_t) proc_timer_tick);

        Sys::CPU::broadcastPacket(Sys::CPU::IPP_CALL, (void*) reload);
        reload();
    }

    extern "C" void proc_timer_tick_ext() {
        auto cpu = Sys::CPU::getCurrent();
        cpu->in_interrupt++;

        schedule();
        Sys::APIC::eoi();

        cpu->in_interrupt--;
    }

    extern "C" void proc_reshed_manual_ext() {
        auto cpu = Sys::CPU::getCurrent();
        cpu->in_interrupt++;

        schedule();

        cpu->in_interrupt--;
    }
}