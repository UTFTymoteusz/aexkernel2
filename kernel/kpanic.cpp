#include "kernel/kpanic.hpp"

#include "kernel/printk.hpp"
#include "sys/cpu.hpp"

#include <stdarg.h>

namespace AEX {
    void kpanic(const char* format, ...) {
        va_list args;
        va_start(args, format);

        printk("Kernel Panic\n", args);
        printk(format, args);

        va_end(args);

        Sys::CPU::halt();
    }
}