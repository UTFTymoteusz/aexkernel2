#include "proc/proc.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/arch/sys/cpu/idt.hpp"
#include "aex/printk.hpp"

#include "sys/irq.hpp"
#include "sys/irq/apic.hpp"

using CPU  = AEX::Sys::CPU;
using APIC = AEX::Sys::IRQ::APIC;

namespace AEX::Proc {
    extern "C" void proc_timer_tick();
    extern "C" void proc_sched_nosave_int();
    extern "C" void proc_ctxsave_int();
    extern "C" void proc_ctxload_int();

    void reload_idt() {
        Sys::load_idt(Sys::idt, 256);
    }

    void setup_irq() {
        Sys::idt[0x20].setOffset((size_t) proc_timer_tick).setIST(3);
        Sys::idt[0x70].setOffset((size_t) proc_timer_tick).setIST(3).setPresent(true);
        Sys::idt[0x71].setOffset((size_t) proc_sched_nosave_int).setIST(3).setPresent(true);
        Sys::idt[0x72].setOffset((size_t) proc_ctxsave_int).setIST(4).setPresent(true);
        Sys::idt[0x73].setOffset((size_t) proc_ctxload_int).setIST(3).setPresent(true);

        CPU::broadcast(CPU::IPP_CALL, (void*) reload_idt);
        reload_idt();
    }

    extern "C" void proc_timer_tick_ext() {
        auto cpu = CPU::current();
        cpu->in_interrupt++;

        schedule();
        APIC::eoi();

        cpu->in_interrupt--;
    }

    extern "C" void proc_sched_int_ext() {
        auto cpu = CPU::current();
        cpu->in_interrupt++;

        schedule();

        cpu->in_interrupt--;
    }
}