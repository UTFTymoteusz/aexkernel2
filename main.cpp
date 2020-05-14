#include "aex/mem/heap.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"

#include "boot/mboot.h"
#include "cpu/idt.hpp"
#include "dev/dev.hpp"
#include "kernel/acpi/acpi.hpp"
#include "mem/memory.hpp"
#include "proc/proc.hpp"
#include "sys/cpu.hpp"
#include "sys/irq.hpp"
#include "sys/mcore.hpp"
#include "tty.hpp"

using namespace AEX;
using namespace AEX::Sys;

namespace AEX::Init {
    void init_print_header();
}

void main_threaded();

void main(multiboot_info_t* mbinfo) {
    TTY::init();

    Init::init_print_header();
    printk(PRINTK_INIT "Booting AEX/0.01\n\n");

    // clang-format off
    printk("Section info:\n");
    printk(".text  : %93$0x%p%90$, %93$0x%p%$\n", &_start_text  , &_end_text);
    printk(".rodata: %93$0x%p%90$, %93$0x%p%$\n", &_start_rodata, &_end_rodata);
    printk(".data  : %93$0x%p%90$, %93$0x%p%$\n", &_start_data  , &_end_data);
    printk(".bss   : %93$0x%p%90$, %93$0x%p%$\n", &_start_bss   , &_end_bss);
    printk("\n");
    // clang-format on

    PMem::init(mbinfo);
    VMem::init();
    Heap::init();
    printk("\n");

    ACPI::init();
    printk("\n");

    IRQ::init();
    IRQ::init_timer();
    printk("\n");

    Dev::init();
    printk("\n");

    auto bsp = new CPU(0);
    bsp->initLocal();

    MCore::init();

    CPU::interrupts();
    IRQ::setup_timers_mcore(250);

    Proc::init();

    VMem::cleanup_bootstrap();
    printk("\n");

    // Let's get to it
    main_threaded();
}

void second_thread() {
    auto idle    = Proc::processes.get(0);
    auto process = Proc::Thread::getCurrentThread()->getProcess();

    process->lock.acquire();

    printk("threads\n");
    for (int i = 0; i < process->threads.count(); i++) {
        printk("  tid %i\n", process->threads[i]);
    }

    process->lock.release();

    Proc::Thread::sleep(10000);
}

void main_threaded() {
    auto idle    = Proc::processes.get(0);
    auto process = Proc::Thread::getCurrentThread()->getProcess();

    auto thread = new Proc::Thread(process.get(), (void*) second_thread, new uint8_t[1024], 1024,
                                   process->pagemap);
    thread->start();

    Proc::Thread::sleep(500);

    while (true) {
        uint64_t ns = IRQ::get_uptime();

        printk("%i: %li (%li ms, %li s, %li min)\n", CPU::getCurrentCPUID(), ns, ns / 1000000,
               ns / 1000000000, ns / 1000000000 / 60);
        printk("idle: %li ns (%li ms) cpu time (pid %i)\n", idle->usage.cpu_time_ns,
               idle->usage.cpu_time_ns / 1000000, idle->pid);
        printk("us  : %li ns (%li ms) cpu time (pid %i)\n", process->usage.cpu_time_ns,
               process->usage.cpu_time_ns / 1000000, process->pid);

        printk("tid: %i\n", Proc::Thread::getCurrentTID());

        Proc::Thread::sleep(1000);
    }
}