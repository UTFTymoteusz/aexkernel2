#pragma once

namespace AEX {
    __attribute((noreturn)) void kpanic(const char* format, ...);
}