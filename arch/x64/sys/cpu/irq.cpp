#include "sys/cpu/irq.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/sys/irq.hpp"
#include "aex/tty.hpp"

#include "sys/irq/apic.hpp"

extern "C" void common_irq_handler(AEX::Sys::CPU::irq_info* info);

namespace AEX::Sys::IRQ {
    volatile bool irq_mark = false;

    extern "C" void common_irq_handler(CPU::irq_info* info) {
        CPU::getCurrent()->in_interrupt++;

        handle(info->irq_no);

        CPU::getCurrent()->in_interrupt--;
        APIC::eoi();
    }

    extern "C" void irq_marker(void*) {
        irq_mark = true;

        APIC::eoi();
    }
}