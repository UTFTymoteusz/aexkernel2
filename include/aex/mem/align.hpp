#pragma once

#include "aex/arch/sys/cpu.hpp"
#include "aex/math.hpp"

namespace AEX::Mem {
    template <typename T>
    bool pagealigned(T val) {
        return !(val & (Sys::CPU::PAGE_SIZE - 1));
    }

    template <typename T>
    T pagefloor(T val) {
        return int_floor<T>(val, Sys::CPU::PAGE_SIZE);
    }

    template <typename T>
    T pageceil(T val) {
        return int_ceil<T>(val, Sys::CPU::PAGE_SIZE);
    }
}
