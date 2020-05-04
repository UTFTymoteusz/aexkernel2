#include "aex/mem/heap.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"
#include "aex/vector.hpp"

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

namespace AEX::Init {
    void init_print_header();
}

void main(multiboot_info_t* mbinfo) {
    TTY::init();

    Init::init_print_header();
    printk(PRINTK_INIT "Booting AEX/0.01\n\n");

    // clang-format off
    printk("Section info:\n");
    printk(".text  : %93$0x%p%90$, %93$0x%p%97$\n", &_start_text  , &_end_text);
    printk(".rodata: %93$0x%p%90$, %93$0x%p%97$\n", &_start_rodata, &_end_rodata);
    printk(".data  : %93$0x%p%90$, %93$0x%p%97$\n", &_start_data  , &_end_data);
    printk(".bss   : %93$0x%p%90$, %93$0x%p%97$\n", &_start_bss   , &_end_bss);
    printk("\n");
    // clang-format on

    PMem::init(mbinfo);
    VMem::init();
    Heap::init();
    printk("\n");

    ACPI::init();
    printk("\n");

    Sys::IRQ::init();
    Sys::IRQ::init_timer();
    printk("\n");

    // Sys::PCI::init();
    // printk("\n");

    Dev::init();
    printk("\n");

    auto bsp = new Sys::CPU(0);
    bsp->initLocal();

    Sys::MCore::init();

    Sys::CPU::interrupts();

    Sys::IRQ::setup_timers_mcore(250);

    Proc::init();

    auto idle    = Proc::processes.get(0);
    auto process = Proc::Thread::getCurrentThread()->getProcess();

    process->cpu_affinity.mask(2, true);
    process->cpu_affinity.mask(3, true);

    while (true) {
        uint64_t ns = Sys::IRQ::get_curtime();

        printk("%i: %li (%li ms, %li s, %li min)\n", Sys::CPU::getCurrentCPUID(), ns, ns / 1000000,
               ns / 1000000000, ns / 1000000000 / 60);
        printk("idle: %li ns (%li ms) cpu time (pid %i)\n", idle->usage.cpu_time_ns,
               idle->usage.cpu_time_ns / 1000000, idle->pid);
        printk("us  : %li ns (%li ms) cpu time (pid %i)\n", process->usage.cpu_time_ns,
               process->usage.cpu_time_ns / 1000000, process->pid);

        Proc::Thread::sleep(5000);
    }
}