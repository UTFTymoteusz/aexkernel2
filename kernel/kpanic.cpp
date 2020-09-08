#include "aex/kpanic.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/printk.hpp"

#include "proc/proc.hpp"

#include <stdarg.h>

using CPU = AEX::Sys::CPU;

namespace AEX {
    void kpanic(const char* format, ...) {
        va_list args;
        va_start(args, format);

        CPU::nointerrupts();

        printk_fault();
        printk(PRINTK_FAIL "Kernel Panic (cpu%i)\n", CPU::currentID());
        printk(format, args);
        printk("\n");

        if (Proc::ready)
            Proc::debug_print_cpu_jobs();

        printk("Stack trace:\n");
        Debug::stack_trace(1);

        va_end(args);

        CPU::broadcast(CPU::IPP_HALT);
        CPU::halt();
    }
}