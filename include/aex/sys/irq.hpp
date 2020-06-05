#pragma once

#include <stdint.h>

namespace AEX::Sys::IRQ {
    void handle_irq(uint8_t irq);

    void register_immediate_handler(uint8_t irq, void (*func)(void* arg), void* arg = nullptr);
    void register_threaded_handler(uint8_t irq, void (*func)(void* arg), void* arg = nullptr);
}