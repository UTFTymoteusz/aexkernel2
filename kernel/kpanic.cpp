#include "aex/kpanic.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/dev/tty.hpp"
#include "aex/printk.hpp"

#ifdef BSOD_PARODY
#include "aex/dev/tty/vtty.hpp"
#endif

#include "proc/proc.hpp"

#include <stdarg.h>

using CPU = AEX::Sys::CPU;

namespace AEX {
    Hook<void (*)()> kpanic_hook;

    void kpanic(const char* format, ...) {
        static int panicked = 0;

        va_list args;
        va_start(args, format);

        CPU::nointerrupts();

        char buffer[512];
        snprintf(buffer, sizeof(buffer), format, args);

        CPU::current()->pushFmsg(buffer);

        if (Mem::atomic_fetch_add(&panicked, 1) != 0) {
            printk(FAIL "recursive kpanic, wonderful (cpu%i, pid%i, th%p)\n", CPU::currentID(),
                   CPU::current()->current_thread->parent->pid, CPU::current()->current_thread);

            Debug::stack_trace();
            CPU::halt();
        }

        printk_fault();

#ifdef BSOD_PARODY
        (*Dev::TTY::VTTYs[0]).setCursorX(0).setCursorY(0).color(Dev::TTY::ANSI_BG_BLUE).clear();

        printk("\r\nA problem has been detected and AEX/2 has been shut down to prevent damage\n");
        printk("\rto your computer.\n");
        printk("\r\n");

        printk("\rIf this is the first time you have seen this Stop error screen,\n");
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

        printk(FAIL "Kernel Panic (cpu%i, pid%i, th%p)\n", CPU::currentID(),
               CPU::current()->current_thread->parent->pid, CPU::current()->current_thread);
        printk("%s", buffer);
        printk("\n");

        va_end(args);

        kpanic_hook.invoke();

        CPU::broadcast(CPU::IPP_HALT);
        CPU::halt();
    }

    void kcalmness(const char* format, ...) {
        static int panicked = 0;

        va_list args;
        va_start(args, format);

        CPU::nointerrupts();

        if (Mem::atomic_add_fetch(&panicked, 1) > 20)
            while (true)
                CPU::wait();

        printk_fault();

        printk(OK "Kernel Calmness (cpu%i, pid%i, th%p)\n", CPU::currentID(),
               CPU::current()->current_thread->parent->pid, CPU::current()->current_thread);
        printk(format, args);
        printk("\n");

        va_end(args);

        kpanic_hook.invoke();

        CPU::broadcast(CPU::IPP_HALT);
        CPU::halt();
    }
}