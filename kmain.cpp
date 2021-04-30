#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/dev/input.hpp"
#include "aex/dev/tty.hpp"
#include "aex/dev/tty/tty.hpp"
#include "aex/fs.hpp"
#include "aex/ipc/pipe.hpp"
#include "aex/mem.hpp"
#include "aex/module.hpp"
#include "aex/net.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/proc/exec.hpp"
#include "aex/sys/acpi.hpp"
#include "aex/sys/irq.hpp"
#include "aex/sys/power.hpp"
#include "aex/sys/time.hpp"

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
#include "sys/irq_i.hpp"
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

void kmain_env();

void init_mem(multiboot_info_t* mbinfo);
void mount_fs();

extern "C" void kmain(multiboot_info_t* mbinfo) {
    // Dirty workaround but meh, it works
    CPU::current()->in_interrupt++;

    Dev::TTY::init(mbinfo);
    Init::print_header();
    printk(PRINTK_INIT "Booting AEX/2 on " ARCH ", build " VERSION "\n\n");

    AEX_ASSERT(mbinfo->flags & MULTIBOOT_INFO_MODS);

    init_mem(mbinfo);
    load_symbols(mbinfo);

    AEX_ASSERT(Debug::symbols_loaded);

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

    mount_fs();

    IPC::init();
    Net::init();
    printk("\n");

    Sys::syscall_init();
    Dev::Input::init();

    load_core_modules();

    // Let's get to it
    kmain_env();
}

void boi(const char*) {
    for (int i = 0; i < 2; i++) {
        Proc::debug_print_cpu_jobs();
        Proc::Thread::sleep(2000);
    }
}

void init_mem(multiboot_info_t* mbinfo) {
    // clang-format off
    printk("Section info:\n");
    printk(".text  : %93$0x%p%90$, %93$0x%p%$\n", &_start_text  , &_end_text);
    printk(".rodata: %93$0x%p%90$, %93$0x%p%$\n", &_start_rodata, &_end_rodata);
    printk(".data  : %93$0x%p%90$, %93$0x%p%$\n", &_start_data  , &_end_data);
    printk(".bss   : %93$0x%p%90$, %93$0x%p%$\n", &_start_bss   , &_end_bss);
    printk("\n");
    // clang-format on

    Phys::init(mbinfo);
    Mem::init();
    Heap::init();
    printk("\n");

    Dev::TTY::init_mem(mbinfo);
}

void mount_fs() {
    FS::mount(nullptr, "/dev/", "devfs");

    auto res = FS::mount("/dev/sra", "/", nullptr);
    if (res != ENONE)
        kpanic("Failed to mount iso9660: %s", strerror((error_t) res));

    printk("\n");
}

void exec_init() {
    auto tty_rd = FS::File::open("/dev/tty0", FS::O_RD);
    AEX_ASSERT(tty_rd);

    auto tty_wr = FS::File::open("/dev/tty0", FS::O_WR);
    AEX_ASSERT(tty_wr);

    auto tty_wre = tty_wr.value->dup();
    AEX_ASSERT(tty_wre);

    FS::File_SP rp, wp;
    IPC::Pipe::create(rp, wp);

    auto info = Proc::exec_opt{
        .stdin  = tty_rd.value,
        .stdout = tty_wr.value,
        .stderr = tty_wre.value,
    };

    char* argv[2] = {
        (char*) "aexinit",
        nullptr,
    };

    char* envp[3] = {
        (char*) "PATH=/bin/:/usr/bin/",
        (char*) "DOES_THIS_WORK=yes",
        nullptr,
    };

    int status = 0;

    AEX_ASSERT(Proc::exec(nullptr, nullptr, "/sys/aexinit", argv, envp, &info) == ENONE);
    Proc::Process::wait(status);

    printk("init exited with a code %i\n", status);
}

void kmain_env() {
    using namespace AEX::Sys::Time;

    exec_init();

    AEX_ASSERT(Sys::Power::poweroff());

    printk(PRINTK_OK "mm it works\n");
    Proc::Thread::sleep(100);

    Proc::processes_lock.acquire();

    auto idle    = Proc::get_process(0);
    auto process = Proc::Process::current();

    Proc::processes_lock.release();

    time_t start_epoch = clocktime();

    while (true) {
        switch (Dev::TTY::TTYs[Dev::TTY::ROOT_TTY]->read()) {
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
            break;
        }
    }

    Proc::Thread::exit();
}