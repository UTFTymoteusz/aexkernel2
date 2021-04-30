#pragma once

#include "aex/utility.hpp"

#include <stddef.h>
#include <stdint.h>

namespace AEX::Sys::IRQ {
    void handle(uint8_t irq);

    API void register_handler(uint8_t irq, void (*func)(void* arg), void* arg = nullptr);
}