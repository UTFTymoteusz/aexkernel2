#pragma once

#include "aex/utility.hpp"

namespace AEX {
    [[noreturn]] API void kpanic(const char* format, ...);
}