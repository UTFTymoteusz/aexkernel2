#pragma once

#include "sys/cpu.hpp"

namespace AEX::Sys::MCore {
    extern int cpu_count;
    extern CPU* CPUs[64];

    void init();
}