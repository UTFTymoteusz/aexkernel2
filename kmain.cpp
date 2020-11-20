#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/dev/input.hpp"
#include "aex/dev/tty.hpp"
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
#include "kernel/module.hpp"
#include "mem/heap.hpp"
#include "mem/phys.hpp"
#include "mem/sections.hpp"
#include "net/net.hpp"
#include "proc/proc.hpp"
#include "sys/cpu/idt.hpp"
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

void kmain_threaded();

void init_mem(multiboot_info_t* mbinfo);
void mount_fs();

extern "C" void kmain(multiboot_info_t* mbinfo) {
    // Dirty workaround but meh, it works
    CPU::current()->in_interrupt++;

    Dev::TTY::init(mbinfo);
    Init::print_header();
    printk(PRINTK_INIT "Booting AEX/2 on " ARCH ", build " VERSION "\n\n");

    if (mbinfo->flags & MULTIBOOT_INFO_MODS) {
        init_mem(mbinfo);
        load_symbols(mbinfo);
    }

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
    IRQ::setup_timers_mcore(500);

    Proc::init();

    bsp->in_interrupt--;

    Dev::init();
    printk("\n");
    FS::init();
    printk("\n");

    load_modules(mbinfo);

    Mem::cleanup_bootstrap();
    printk("\n");

    mount_fs();

    Net::init();
    printk("\n");

    Sys::syscall_init();
    Dev::Input::init();

    load_core_modules();

    // Let's get to it
    kmain_threaded();
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

struct blocc {
    Net::Socket_SP socket;
};

void test_server_handle(blocc* b) {
    auto sock2 = b->socket;
    delete b;

    char buffer[4];
    sock2->receive(buffer, 4, 0);

    sock2->send("HTTP/1.1 200 OK\r\nContent-Length: 16\r\n\r\n<h1>works</h1>\r\n", 56, 0);

    Proc::Thread::sleep(1000);
    sock2->shutdown(Net::SHUT_RDWR);

    Proc::Thread::sleep(1000);
    return;
}

extern "C" void context_test();

void test_client() {
    auto sock_try = Net::Socket::create(Net::AF_INET, Net::SOCK_STREAM, Net::IPROTO_TCP);
    auto sock     = sock_try.value;

    auto error = sock->connect(Net::ipv4_addr(127, 0, 0, 1), 7654);
    if (error) {
        printk("retrying client\n");
        sock->connect(Net::ipv4_addr(127, 0, 0, 1), 7654);
    }
    else
        printk("connected client\n");

    int total = 0;

    char buffer[64];

    auto ret = sock->send(
        "GET /login HTTP/1.1\r\nHost: 127.0.0.1\r\ncookie: "
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n\r\n",
        745, 0);

    if (!ret)
        printk("socket send error: %s\n", strerror(ret));

    while (true) {
        auto ret = sock->receive(buffer, 63, 0);
        if (!ret) {
            printk("socket receive error: %s\n", strerror(ret));
            break;
        }

        total += ret.value;

        if (ret.value == 0) {
            printk("\nnothing more to read\n");
            break;
        }

        buffer[ret.value] = '\0';
        printk("%s", buffer, ret.value);

        if (total == 6165)
            break;
    }

    sock->shutdown(Net::SHUT_RDWR);

    Proc::Thread::sleep(5000);

    sock->close();
}

void test_server() {
    auto sock_try = Net::Socket::create(Net::AF_INET, Net::SOCK_STREAM, Net::IPROTO_TCP);
    if (!sock_try) {
        printk("failed to create() the socket: %s\n", strerror(sock_try));
        return;
    }

    auto sock = sock_try.value;

    auto error = sock->bind(Net::ipv4_addr(127, 0, 0, 1), 7654);
    if (error) {
        printk("failed to bind() the socket: %s\n", strerror(error));
        return;
    }

    error = sock->listen(2);
    if (error) {
        printk("failed to listen() the socket: %s\n", strerror(error));
        return;
    }

    for (size_t i = 0; i < 8; i++) {
        auto sock2_try = sock->accept();
        if (!sock2_try) {
            printk("failed to accept() the socket: %s\n", strerror(sock2_try));
            return;
        }

        auto sock2 = sock2_try.value;
        auto b     = new blocc();

        b->socket = sock2;

        printk("accepted\n");
        auto thread = Proc::threaded_call(test_server_handle, b);
        thread->detach();
    }

    Proc::Thread::sleep(1250);
}

void test_udp_client() {
    auto sock_try = Net::Socket::create(Net::AF_INET, Net::SOCK_DGRAM, Net::IPROTO_UDP);
    if (!sock_try) {
        printk("failed to create udp socket: %s\n", strerror(sock_try));
        return;
    }

    auto sock  = sock_try.value;
    auto error = sock->connect(Net::ipv4_addr(255, 255, 255, 255), 27015);
    if (error) {
        printk("failed to connect udp socket: %s\n", strerror(error));
        return;
    }

    for (size_t i = 0; i < 5; i++) {
        auto send_try = sock->send("\xFF\xFF\xFF\xFFTSource Engine Query", 25, 0);
        if (!send_try) {
            printk("failed to send via udp socket: %s\n", strerror(send_try));
            return;
        }

        char buffer[64] = {};

        auto receive_try = sock->receive(buffer, sizeof(buffer) - 1, 0);
        if (!receive_try) {
            printk("failed to receive via udp socket: %s\n", strerror(receive_try));
            return;
        }

        printk("udp received: %s\n", buffer);

        Proc::Thread::sleep(2500);
    }
}

void exec_init() {
    auto tty_rd = FS::File::open("/dev/tty0", FS::O_RD);
    AEX_ASSERT(tty_rd);

    auto tty_wr = FS::File::open("/dev/tty0", FS::O_WR);
    AEX_ASSERT(tty_wr);

    // auto tty_wre = tty_wr.value->dup();
    // AEX_ASSERT(tty_wre);

    FS::File_SP rp, wp;
    IPC::Pipe::create(rp, wp);

    auto info = Proc::exec_opt{
        .stdin  = tty_rd.value,
        .stdout = tty_wr.value,
        //.stderr = tty_wre.value,
        .stderr = wp,
    };

    int status;

    AEX_ASSERT(Proc::exec("/sys/aexinit.elf", &info) == ENONE);
    Proc::Process::wait(status);

    printk("init exited with a code %i\n", status);

    char buffer[33];
    rp->read(buffer, sizeof(buffer));

    for (size_t i = 0; i < sizeof(buffer); i++)
        Dev::TTY::VTTYs[0]->write(buffer[i]);
}

void kmain_threaded() {
    using namespace AEX::Sys::Time;

    // Proc::processes_lock.acquire();

    // auto idle    = Proc::get_process(0);
    // auto process = Proc::Process::current();

    // Proc::processes_lock.release();

    // time_t start_epoch = clocktime();

    exec_init();

    AEX_ASSERT(Power::poweroff());

    /*printk(PRINTK_OK "mm it works\n");
    Proc::Thread::sleep(100);

    while (true) {
        switch (Dev::TTY::VTTYs[Dev::TTY::ROOT_TTY]->read()) {
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
        default:
            break;
        }
    }

    Proc::Thread::exit();*/
}