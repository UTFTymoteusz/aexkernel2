#include "cpu/irq.hpp"

#include "aex/printk.hpp"

#include "sys/apic.hpp"
#include "sys/cpu.hpp"
#include "tty.hpp"

extern "C" void common_irq_handler(void* info);

namespace AEX::Sys::IRQ {
    volatile bool irq_mark = false;

    extern "C" void common_irq_handler(void* _info) {
        auto info = (CPU::irq_info_t*) _info;

        AEX::printk("%i: irq: %i\n", CPU::getCurrentCPUID(), info->irq_no);

        APIC::eoi();
    }

    extern "C" void irq_marker(void*) {
        irq_mark = true;

        APIC::eoi();
    }
}