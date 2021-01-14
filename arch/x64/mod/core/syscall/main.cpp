#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/printk.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/irq.hpp"
#include "aex/sys/syscall.hpp"
#include "aex/types.hpp"

#include <syscallids.h>

using namespace AEX;

namespace AEX::Sys {
    struct idt_entry {
        uint16_t offset_0;

        uint16_t selector;
        uint8_t  ist;
        uint8_t  attributes;

        uint16_t offset_1;
        uint32_t offset_2;

        uint32_t zero;

        idt_entry& setOffset(size_t offset);
        idt_entry& setOffset(void* offset);
        idt_entry& setType(uint8_t type);
        idt_entry& setSelector(uint8_t selector);
        idt_entry& setPresent(bool present);
        idt_entry& setIST(uint8_t ist);
    } __attribute((packed));

    extern idt_entry idt[256];
}

constexpr auto IA32_STAR  = 0xC0000081;
constexpr auto IA32_LSTAR = 0xC0000082;

const char* MODULE_NAME = "syscall";

extern "C" void handler(uint64_t id);
extern "C" void int_handler(void*);
void            register_syscalls();
void            install_handler();

void register_fs();
void register_proc();
void register_ipc();
void register_test();

int sigact(int signum, const IPC::sigaction_usr* act, IPC::sigaction_usr* oldact);

void module_enter() {
    Sys::CPU::broadcast(Sys::CPU::IPP_CALL, (void*) install_handler, false);
    register_syscalls();

    auto table = Sys::default_table();

    // -O2 makes this assert pass
    AEX_ASSERT(table[SYS_SIGACT] == (void*) sigact);
}

void module_exit() {
    //
}

void install_handler() {
    Sys::CPU::wrmsr(IA32_STAR, (Sys::CPU::rdmsr(IA32_STAR) & 0xFFFFFFFF) | (0x00100008ul << 32));
    Sys::CPU::wrmsr(IA32_LSTAR, (uint64_t) handler);

    AEX::Sys::idt[0x80]
        .setOffset((void*) int_handler)
        .setSelector(0x08)
        .setType(0x6F)
        .setIST(7)
        .setPresent(true);
}

void register_syscalls() {
    register_fs();
    register_proc();
    register_ipc();
    register_test();
}

extern "C" void syscall_prepare() {
    Proc::Thread::current()->addBusy();
}

extern "C" void syscall_done() {
    Proc::Thread::current()->subBusy();
    AEX_ASSERT(!Proc::Thread::current()->isBusy());
}