#include "aex/kpanic.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/dev/tty.hpp"
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

        if (Mem::atomic_add_fetch(&panicked, 1) > 20)
            while (true)
                CPU::wait();

        printk_fault();

#ifdef BSOD_PARODY
        (*Dev::TTY::VTTYs[0]).setCursorX(0).setCursorY(0).color(Dev::TTY::ANSI_BG_BLUE).clear();

        printk("\r\nA problem has been detected and AEX/2 has been shut down to prevent damage\n");
        printk("\rto your computer.\n");
        printk("\r\n");

        printk("\rIf this is the first time you've seen this Stop error screen,\n");
        printk("\rrestart your computer. If this screen appears again, follow\n");
        printk("\rthese steps:\n");
        printk("\r\n");

        printk("\rCheck to make sure any new hardware or software is properly installed.\n");
        printk("\rIf this is a new installation, ask your hardware or software manufacturer\n");
        printk("\rfor any AEX/2 updates you might need.\n");
        printk("\r\n");

        printk("\rIf problems continue, disable or remove any newly installed hardware\n");
        printk("\ror software. Disable BIOS memory options such as caching or shadowing.\n");
        printk("\r\n");

        printk("\rTechnical information:\n\n");
#endif

        printk(PRINTK_FAIL "Kernel Panic (cpu%i)\n", CPU::currentID());
        printk(format, args);
        printk("\n");

        if (Proc::ready)
            Proc::debug_print_threads();

        printk("Stack trace:\n");
        Debug::stack_trace(1);

        va_end(args);

        CPU::broadcast(CPU::IPP_HALT);
        CPU::halt();
    }
}