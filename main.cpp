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

    auto bsp = new Sys::CPU(0);
    bsp->initLocal();

    Sys::IRQ::setup_timer();

    Sys::CPU::interrupts();

    /*Sys::CPU::globalInit();

    ACPI::init();
    printk("\n");

    auto cpu            = new Sys::CPU(0);
    Sys::MCore::CPUs[0] = cpu;

    Sys::MCore::init();

    Sys::CPU::broadcastPacket(3, nullptr, false);

    while (true) {
        //printk("0x%x\n", Sys::IOAPIC::read(0x10 + 4));
        asm volatile("hlt;");

    }*/

    while (true)
        asm volatile("hlt;");
}