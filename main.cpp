#include "aex/fs/fs.hpp"
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
#include "fs/fs.hpp"
#include "kernel/acpi/acpi.hpp"
#include "mem/memory.hpp"
#include "proc/proc.hpp"
#include "sys/cpu.hpp"
#include "sys/irq.hpp"
#include "sys/mcore.hpp"
#include "tty.hpp"

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

    IRQ::init();
    IRQ::init_timer();
    printk("\n");

    auto bsp = new CPU(0);
    bsp->initLocal();

    Dev::init();
    printk("\n");

    MCore::init();

    CPU::interrupts();
    IRQ::setup_timers_mcore(250);

    Proc::init();

    VMem::cleanup_bootstrap();
    printk("\n");

    FS::init();
    FS::mount(nullptr, "/dev/", "devfs");

    auto res = FS::mount("/dev/sra", "/", nullptr);
    if (res != error_t::ENONE)
        printk("Failed to mount iso9660: %s\n", strerror((error_t) res));

    auto res2 = FS::File::info("/sys/aexkrnl.elf");
    if (!res2.has_value)
        printk("Failed to open /sys/aexkrnl.elf: %s\n", strerror(res2.error_code));
    else
        printk("/sys/aexkrnl.elf total size: %i\n", res2.value.total_size);

    printk("\n");

    auto dir = FS::File::opendir("/sys/");
    if (dir.has_value) {
        printk("Opened dir\n");

        auto bong = dir.value->readdir();
        while (bong.has_value) {
            printk(" - %s %s\n", bong.value.name, bong.value.is_directory() ? "dir" : "file");
            bong = dir.value->readdir();
        }

        printk("/dev/sra id: %i\n", FS::File::info("/dev/sra").value.dev_id);
    }
    else
        printk("Failed to open dir: %s\n", strerror(dir.error_code));

    auto file = FS::File::open("/sys/test.txt");
    if (file.has_value) {
        char buffer[24] = {};

        file.value->seek(20);

        int amnt = file.value->read(buffer, 20).value;
        printk("read: %s (%i)\n", buffer, amnt);
    }
    else
        printk("Failed to open /sys/test.txt: %s\n", strerror(file.error_code));

    // Let's get to it
    main_threaded();
}

IPC::MessageQueue* mqueue;

void secondary_threaded() {
    char buffer[16];
    mqueue->readArray(buffer, 9);

    printk("received: %s\n", buffer);

    Proc::Thread::sleep(1200);
}

void main_threaded() {
    auto idle    = Proc::processes.get(0);
    auto process = Proc::Thread::getCurrentThread()->getProcess();

    mqueue = new IPC::MessageQueue();

    auto thread = new Proc::Thread(process.get(), (void*) secondary_threaded, Heap::malloc(8192),
                                   8192, process->pagemap);
    thread->start();

    Proc::Thread::sleep(500);

    mqueue->writeObject("xdxdddxd");

    thread->join();
    printk("joined\n");

    while (true) {
        uint64_t ns = IRQ::get_uptime();

        printk("cpu%i: %16li ns (%li ms, %li s, %li min)\n", CPU::getCurrentCPUID(), ns,
               ns / 1000000, ns / 1000000000, ns / 1000000000 / 60);
        printk("idle: %16li ns (%li ms) cpu time (pid %i)\n", idle->usage.cpu_time_ns,
               idle->usage.cpu_time_ns / 1000000, idle->pid);
        printk("us  : %16li ns (%li ms) cpu time (pid %i)\n", process->usage.cpu_time_ns,
               process->usage.cpu_time_ns / 1000000, process->pid);

        printk("tid: %i\n", Proc::Thread::getCurrentTID());

        Proc::debug_print_cpu_jobs();

        Proc::Thread::sleep(5000);
    }
}