#include "aex/arch/sys/cpu.hpp"
#include "aex/debug.hpp"
#include "aex/fs/fs.hpp"
#include "aex/mem/heap.hpp"
#include "aex/mem/pmem.hpp"
#include "aex/mem/smartarray.hpp"
#include "aex/mem/smartptr.hpp"
#include "aex/mem/vector.hpp"
#include "aex/mem/vmem.hpp"
#include "aex/module.hpp"
#include "aex/net/ipv4.hpp"
#include "aex/net/socket.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/irq.hpp"
#include "aex/sys/time.hpp"
#include "aex/tty.hpp"

#include "boot/mboot.h"
#include "cpu/idt.hpp"
#include "dev/dev.hpp"
#include "fs/fs.hpp"
#include "kernel/acpi/acpi.hpp"
#include "kernel/module.hpp"
#include "mem/memory.hpp"
#include "net/net.hpp"
#include "proc/proc.hpp"
#include "sys/irq.hpp"
#include "sys/irq_i.hpp"
#include "sys/mcore.hpp"
#include "sys/time.hpp"

// clang-format off
#include "aex/ipc/messagequeue.hpp"
// clang-format on

using namespace AEX;
using namespace AEX::Sys;

namespace AEX::Init {
    void init_print_header();
}

void main_threaded();

void main(multiboot_info_t* mbinfo) {
    Sys::CPU::getCurrentCPU()->in_interrupt++;

    TTY::init(mbinfo);

    Init::init_print_header();
    printk(PRINTK_INIT "Booting AEX/2\n\n");

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

    TTY::init_mem(mbinfo);

    ACPI::init();
    printk("\n");

    Sys::init_time();
    printk("\n");

    IRQ::init();
    IRQ::init_timer();
    printk("\n");

    auto bsp = new CPU(0);
    bsp->initLocal();

    Dev::init();
    printk("\n");

    MCore::init();

    CPU::interrupts();
    IRQ::setup_timers_mcore(500);

    bsp->in_interrupt--;

    Proc::init();

    VMem::cleanup_bootstrap();
    printk("\n");

    IRQ::init_proc();

    FS::init();
    FS::mount(nullptr, "/dev/", "devfs");

    auto res = FS::mount("/dev/sra", "/", nullptr);
    if (res != error_t::ENONE)
        kpanic("Failed to mount iso9660: %s\n", strerror((error_t) res));

    printk("\n");

    Net::init();
    printk("\n");

    Debug::load_kernel_symbols("/sys/aexkrnl.elf");
    load_core_modules();

    // Let's get to it
    main_threaded();
}

void main_threaded() {
    auto idle    = Proc::processes.get(0);
    auto process = Proc::Thread::getCurrentThread()->getProcess();

    int64_t start_epoch = Sys::get_clock_time();

    Proc::Thread::sleep(1000);

    while (true) {
        uint64_t ns    = get_uptime();
        uint64_t clock = Sys::get_clock_time();

        auto dt = Sys::from_unix_epoch(clock);

        printk("cpu%i: %16li ns (%li ms, %li s, %li min), clock says %li s, %02i:%02i:%02i\n",
               CPU::getCurrentCPUID(), ns, ns / 1000000, ns / 1000000000, ns / 1000000000 / 60,
               clock - start_epoch, dt.hour, dt.minute, dt.second);
        printk("idle: %16li ns (%li ms) cpu time (pid %i)\n", idle->usage.cpu_time_ns,
               idle->usage.cpu_time_ns / 1000000, idle->pid);
        printk("us  : %16li ns (%li ms) cpu time (pid %i)\n", process->usage.cpu_time_ns,
               process->usage.cpu_time_ns / 1000000, process->pid);
        printk("bytes read: %li\n", process->usage.block_bytes_read);
        printk("heap free : %li\n", Heap::heap_free);

        printk("tid: %i\n", Proc::Thread::getCurrentTID());

        Proc::debug_print_cpu_jobs();

        Proc::Thread::sleep(5000);
    }
}