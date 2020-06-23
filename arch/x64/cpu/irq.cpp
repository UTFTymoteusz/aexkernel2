#include "cpu/irq.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/irq.hpp"
#include "aex/tty.hpp"

#include "sys/apic.hpp"

extern "C" void common_irq_handler(AEX::Sys::CPU::irq_info* info);

namespace AEX::Sys::IRQ {
    volatile bool irq_mark = false;

    extern "C" void common_irq_handler(CPU::irq_info* info) {
        CPU::getCurrentCPU()->in_interrupt++;

        // if (info->irq_no > 0)
        //    AEX::printk("%i: irq: %i\n", CPU::getCurrentCPUID(), info->irq_no);

        handle_irq(info->irq_no);

        APIC::eoi();
        CPU::getCurrentCPU()->in_interrupt--;
    }

    extern "C" void irq_marker(void*) {
        irq_mark = true;

        APIC::eoi();
    }
}