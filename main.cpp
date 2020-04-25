#include "boot/mboot.h"
#include "kernel/acpi/acpi.hpp"
#include "kernel/printk.hpp"
#include "mem/heap.hpp"
#include "mem/memory.hpp"
#include "mem/pmem.hpp"
#include "mem/vmem.hpp"
#include "sys/cpu.hpp"
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

    Sys::CPU::globalInit();

    auto cpu = new Sys::CPU(0);
    Sys::MCore::CPUs.addRef(cpu);

    Sys::CPU::interrupts();
    Sys::MCore::init();

    Sys::CPU::broadcastPacket(3, nullptr, false);
}