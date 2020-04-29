#include "aex/mem/heap.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/printk.hpp"

#include "boot/mboot.h"
#include "cpu/idt.hpp"
#include "kernel/acpi/acpi.hpp"
#include "mem/memory.hpp"
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

    Sys::setup_idt();
    Sys::load_idt(Sys::init_IDT, 256);

    PMem::init(mbinfo);
    VMem::init();
    Heap::init();
    printk("\n");

    ACPI::init();
    printk("\n");

    Sys::IRQ::init();
    Sys::IRQ::setup_timer(0.1);
    printk("\n");

    auto bsp = new Sys::CPU(0);
    bsp->initLocal();

    Sys::MCore::init();

    Sys::CPU::interrupts();

    Sys::IRQ::setup_timers_mcore(100);

    while (true)
        Sys::CPU::waitForInterrupt();
}