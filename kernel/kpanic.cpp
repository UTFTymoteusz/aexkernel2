#include "aex/kpanic.hpp"

#include "aex/debug.hpp"
#include "aex/printk.hpp"

#include "sys/cpu.hpp"

#include <stdarg.h>

using CPU = AEX::Sys::CPU;

namespace AEX {
    void kpanic(const char* format, ...) {
        va_list args;
        va_start(args, format);

        printk("Kernel Panic\n", args);
        printk(format, args);
        printk("\n");

        printk("Stack trace:\n");
        Debug::stack_trace(1);

        va_end(args);

        CPU::broadcastPacket(CPU::ipp_type::HALT);
        CPU::halt();
    }
}