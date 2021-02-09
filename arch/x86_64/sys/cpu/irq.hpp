#pragma once

namespace AEX::Sys::IRQ {
    /**
     * A bool that will be set to true when IRQ 31 occurs.
     **/
    extern volatile bool irq_mark;
}