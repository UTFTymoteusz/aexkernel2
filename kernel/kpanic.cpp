#include "aex/kpanic.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/printk.hpp"

#include "proc/proc.hpp"

#include <stdarg.h>

using CPU = AEX::Sys::CPU;

namespace AEX {
    void kpanic(const char* format, ...) {
        static int panicked = false;

        va_list args;
        va_start(args, format);

        CPU::nointerrupts();

        if (Mem::atomic_add_fetch(&panicked, 1) > 2)
            while (true)
                CPU::waitForInterrupt();

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