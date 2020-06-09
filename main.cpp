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
    IRQ::setup_timers_mcore(500);

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
    {
        auto socket_try =
            Net::Socket::create(Net::socket_domain_t::AF_INET, Net::socket_type_t::SOCK_DGRAM,
                                Net::socket_protocol_t::IPROTO_UDP);
        if (!socket_try.has_value)
            kpanic("socket failed: %s\n", strerror(socket_try.error_code));

        auto    socket   = socket_try.value;
        error_t bind_res = socket->bind(Net::ipv4_addr(0, 0, 0, 0), 7654);

        printk("bind: %s\n", strerror(bind_res));

        uint8_t buffer[64] = {};

        Net::sockaddr_inet boii = Net::sockaddr_inet(Net::ipv4_addr(127, 0, 0, 1), 7654);

        while (true) {
            Net::sockaddr_inet addr;

            memcpy(buffer, "\xFF\xFF\xFF\xFFTSource Engine Query", 25);

            auto send_res = socket->sendto(buffer, 25, 0, (Net::sockaddr*) &boii);
            printk("send: %s\n", strerror(send_res.error_code));

            auto ret                   = socket->recvfrom(buffer, 64, 0, (Net::sockaddr*) &addr);
            buffer[sizeof(buffer) - 1] = '\0';

            auto ip_addr = addr.addr;

            printk("received from %i.%i.%i.%i:%i: %s\n", ip_addr[0], ip_addr[1], ip_addr[2],
                   ip_addr[3], addr.port, buffer);

            Proc::Thread::sleep(5000);
        }
    }

    auto idle    = Proc::processes.get(0);
    auto process = Proc::Thread::getCurrentThread()->getProcess();

    Proc::Thread::sleep(1000);

    while (true) {
        uint64_t ns = get_uptime();

        printk("cpu%i: %16li ns (%li ms, %li s, %li min)\n", CPU::getCurrentCPUID(), ns,
               ns / 1000000, ns / 1000000000, ns / 1000000000 / 60);
        printk("idle: %16li ns (%li ms) cpu time (pid %i)\n", idle->usage.cpu_time_ns,
               idle->usage.cpu_time_ns / 1000000, idle->pid);
        printk("us  : %16li ns (%li ms) cpu time (pid %i)\n", process->usage.cpu_time_ns,
               process->usage.cpu_time_ns / 1000000, process->pid);
        printk("bytes read: %li\n", process->usage.block_bytes_read);
        printk("heap free : %li\n", Heap::heap_free);

        printk("tid: %i\n", Proc::Thread::getCurrentTID());

        Proc::debug_print_cpu_jobs();

        Proc::Thread::sleep(500000);
    }
}