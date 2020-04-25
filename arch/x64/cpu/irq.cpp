#include "kernel/printk.hpp"
#include "sys/apic.hpp"

extern "C" void common_irq_handler(void* info);

namespace AEX::Sys {
    extern "C" void common_irq_handler(void* _info) {
        auto info = (CPU::irq_info_t*) _info;

        AEX::printk("%i: irq: %i\n", CPU::getCurrentCPUID(), info->irq_no);

        if (CPU::isAPICPresent)
            APIC::eoi();
        else
            ; // Add legacy PIC EOI'ing
    }
}