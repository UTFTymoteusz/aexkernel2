#include "cpu/irq.hpp"

#include "aex/printk.hpp"
#include "aex/tty.hpp"

#include "sys/apic.hpp"
#include "sys/cpu.hpp"

extern "C" void common_irq_handler(AEX::Sys::CPU::irq_info* info);

namespace AEX::Sys::IRQ {
    volatile bool irq_mark = false;

    extern "C" void common_irq_handler(CPU::irq_info* info) {
        if (info->irq_no > 0)
            AEX::printk("%i: irq: %i\n", CPU::getCurrentCPUID(), info->irq_no);

        APIC::eoi();
    }

    extern "C" void irq_marker(void*) {
        irq_mark = true;

        APIC::eoi();
    }
}