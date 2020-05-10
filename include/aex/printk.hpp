#pragma once

#include <stdarg.h>

#define PRINTK_INIT "^0"
#define PRINTK_OK "^1"
#define PRINTK_WARN "^2"
#define PRINTK_FAIL "^3"

namespace AEX {
    void printk(const char* format, ...);
    void printk(const char* format, va_list args);
}