#include "aex.hpp"
#include "aex/arch/sys/cpu.hpp"
#include "aex/config.hpp"

#include "boot/mboot.h"
#include "dev/dev.hpp"
#include "fs/fs.hpp"
#include "ipc/ipc.hpp"
#include "kernel/module.hpp"
#include "mem/heap.hpp"
#include "mem/phys.hpp"
#include "mem/sections.hpp"
#include "net/net.hpp"
#include "proc/proc.hpp"
#include "sys/acpi.hpp"
#include "sys/irq.hpp"
#include "sys/mcore.hpp"
#include "sys/syscall.hpp"
#include "sys/time.hpp"

using namespace AEX;
using namespace AEX::Mem;
using namespace AEX::Sys;

namespace AEX::Init {
    void print_header();
}

namespace AEX::Dev::Tree {
    void print_debug();
}

void init_mem(multiboot_info_t* mbinfo);
void mount_fs(uint32_t dev_code);
void kmain_env();

extern "C" void kmain(multiboot_info_t* mbinfo) {
    // Dirty workaround but meh, it works
    CPU::current()->in_interrupt++;

    Dev::TTY::init(mbinfo);
    Init::print_header();

    AEX_ASSERT(mbinfo->flags & MULTIBOOT_INFO_MODS);

    init_mem(mbinfo);
    load_symbols(mbinfo);

    AEX_ASSERT(Debug::symbols_loaded);
    uint32_t mb_boot_dev = mbinfo->boot_device;

    ACPI::init();
    printk("\n");

    Time::init();

    IRQ::init();
    IRQ::init_timer();
    printk("\n");

    auto bsp = new CPU(0);
    bsp->initLocal();

    MCore::init();

    CPU::interrupts();
    IRQ::setup_timers_mcore(1000);

    Proc::init();

    bsp->in_interrupt--;

    Dev::init();
    printk("\n");
    FS::init();
    printk("\n");

    load_modules(mbinfo);

    Mem::cleanup();
    printk("\n");

    mount_fs(mb_boot_dev);

    IPC::init();
    Net::init();
    printk("\n");

    Sys::syscall_init();
    Dev::Input::init();

    load_core_modules();

    // Let's get to it
    kmain_env();
}

void init_mem(multiboot_info_t* mbinfo) {
    printk("Section info:\n");
    printk(".text  : %93$0x%p%90$, %93$0x%p%$\n", &_start_text, &_end_text);
    printk(".rodata: %93$0x%p%90$, %93$0x%p%$\n", &_start_rodata, &_end_rodata);
    printk(".data  : %93$0x%p%90$, %93$0x%p%$\n", &_start_data, &_end_data);
    printk(".bss   : %93$0x%p%90$, %93$0x%p%$\n", &_start_bss, &_end_bss);
    printk("\n");

    Phys::init(mbinfo);
    Mem::init();
    Heap::init();
    printk("\n");

    Dev::TTY::init_mem(mbinfo);
}

void mount_fs(uint32_t dev_code) {
    FS::mount(nullptr, "/dev/", "devfs");

    if (dev_code == 0x00000000) {
        printk(WARN "Where we have booted from is one of the greatest mysteries ever\n");
        return;
    }

    char to_try[8][8] = {};

    uint8_t drive     = ((dev_code >> 24) & 0xFF) + 0;
    uint8_t partition = ((dev_code >> 16) & 0xFF) + 1;

    if (drive >= 0xE0) {
        char drive_letter = 'a' + (drive - 0xE0);

        snprintf(to_try[0], 8, "hr%c", drive_letter);
        snprintf(to_try[1], 8, "sr%c", drive_letter);
        strlcpy(to_try[2], "%", 8);
    }
    else if (drive >= 0x80 && drive < 0xE0) {
        char drive_letter = 'a' + (drive - 0x80);

        snprintf(to_try[0], 8, "hd%c%i", drive_letter, partition);
        snprintf(to_try[1], 8, "sd%c%i", drive_letter, partition);
        strlcpy(to_try[2], "%", 8);
    }

    for (int i = 0; i < 8; i++) {
        if (strcmp(to_try[i], "%") == 0)
            break;

        char buffer[256];
        snprintf(buffer, sizeof(buffer), "/dev/%s", to_try[i]);

        if (FS::File::info(buffer) == ENOENT)
            continue;

        if (FS::mount(buffer, "/", nullptr) == ENONE)
            return;
    }

    kpanic("Failed to mount /");
}

void exec_init() {
    auto tty_rd = FS::File::open("/dev/tty0", FS::O_RDONLY);
    AEX_ASSERT(tty_rd);

    auto tty_wr = FS::File::open("/dev/tty0", FS::O_WRONLY);
    AEX_ASSERT(tty_wr);

    auto tty_wre = FS::File::open("/dev/tty0", FS::O_WRONLY);
    AEX_ASSERT(tty_wre);

    auto info = Proc::exec_opt{
        .stdin  = tty_rd.value,
        .stdout = tty_wr.value,
        .stderr = tty_wre.value,
    };

    char* argv[2] = {
        (char*) INIT_PATH,
        nullptr,
    };

    char* envp[3] = {
        (char*) "PATH=/bin/:/usr/bin/",
        (char*) "DOES_THIS_WORK=yes",
        nullptr,
    };

    int status = 0;

    AEX_ASSERT(Proc::exec(nullptr, nullptr, INIT_PATH, argv, envp, &info) == ENONE);
    Proc::Process::wait(status);

    printk(INIT_PATH " exited with status %i\n", status);
}

void apple();

void kmain_env() {
    using namespace AEX::Sys::Time;

    // apple();
    exec_init();

    // Sys::CPU::tripleFault();

    printk("We are done here, adios\n");
    Proc::Thread::sleep(1000);

    AEX_ASSERT(Sys::Power::poweroff());

    printk(OK "mm it works\n");
    Proc::Thread::sleep(100);

    Proc::processes_lock.acquire();

    auto idle    = Proc::get_process(0);
    auto process = Proc::Process::current();

    Proc::processes_lock.release();

    time_t start_epoch = clocktime();

    while (true) {
        char c = Dev::TTY::TTYs[Dev::TTY::ROOT_TTY]->read();

        switch (c) {
        case 't': {
            auto ns    = uptime();
            auto clock = clocktime();

            auto dt = epoch2dt(clock);

            printk("cpu%i: %16li ns (%li ms, %li s, %li min), clock says %li s, %02i:%02i:%02i\n",
                   CPU::currentID(), ns, ns / 1000000, ns / 1000000000, ns / 1000000000 / 60,
                   clock - start_epoch, dt.hour, dt.minute, dt.second);
            printk("idle: %16li ns (%li ms) cpu time (pid %i)\n", idle->usage.cpu_time_ns,
                   idle->usage.cpu_time_ns / 1000000, idle->pid);
            printk("us  : %16li ns (%li ms) cpu time (pid %i)\n", process->usage.cpu_time_ns,
                   process->usage.cpu_time_ns / 1000000, process->pid);
            printk("bytes read: %li\n", process->usage.block_bytes_read);
            printk("heap free : %li\n", Heap::heap_free);

            Proc::debug_print_cpu_jobs();
        } break;
        case 'r':
            CPU::tripleFault();
            break;
        case 'l':
            Proc::debug_print_threads();
            break;
        case 'p':
            Proc::debug_print_processes();
            break;
        case 's':
            AEX_ASSERT(Power::poweroff());
            break;
        default:
            printk("%c", c);
            break;
        }
    }

    Proc::Thread::exit();
}