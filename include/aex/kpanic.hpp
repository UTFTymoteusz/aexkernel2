#pragma once

#include "aex/hook.hpp"
#include "aex/utility.hpp"

namespace AEX {
    extern API Hook<void (*)()> kpanic_hook;

    [[noreturn]] API void kpanic(const char* format, ...);
    [[noreturn]] API void kcalmness(const char* format, ...);
}