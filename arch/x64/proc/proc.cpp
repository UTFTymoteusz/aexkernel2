#include "proc/proc.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/printk.hpp"

#include "sys/cpu/idt.hpp"
#include "sys/irq.hpp"
#include "sys/irq/apic.hpp"

using CPU  = AEX::Sys::CPU;
using APIC = AEX::Sys::IRQ::APIC;

namespace AEX::Proc {
    extern "C" void proc_timer_tick();

    void reload() {
        Sys::load_idt(Sys::idt, 256);
    }

    void setup_irq() {
        Sys::idt[0x20 + 0].setOffset((size_t) proc_timer_tick);

        CPU::broadcast(CPU::IPP_CALL, (void*) reload);
        reload();
    }

    extern "C" void proc_timer_tick_ext() {
        auto cpu = CPU::current();
        cpu->in_interrupt++;

        schedule();
        APIC::eoi();

        cpu->in_interrupt--;
    }

    extern "C" void proc_reshed_manual_ext() {
        auto cpu = CPU::current();
        cpu->in_interrupt++;

        schedule();

        cpu->in_interrupt--;
    }
}