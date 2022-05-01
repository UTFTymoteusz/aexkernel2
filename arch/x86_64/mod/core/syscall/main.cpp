#include "aex/arch/ipc/signal.hpp"
#include "aex/arch/sys/cpu.hpp"
#include "aex/assert.hpp"
#include "aex/debug.hpp"
#include "aex/ipc/signal.hpp"
#include "aex/printk.hpp"
#include "aex/proc/process.hpp"
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

void register_fs(Sys::syscall_t* table);
void register_ipc(Sys::syscall_t* table);
void register_proc(Sys::syscall_t* table);
void register_sec(Sys::syscall_t* table);
void register_net(Sys::syscall_t* table);
void register_test(Sys::syscall_t* table);
void print_all(Sys::syscall_t* table);

int  sys_sigact(int signum, const IPC::sigaction_usr* act, IPC::sigaction_usr* oldact);
void sys_sigathrd(bool asyncthrdsig);
void sys_panic();

void module_enter() {
    Sys::CPU::broadcast(Sys::CPU::IPP_CALL, (void*) install_handler, false);
    register_syscalls();

    auto table = Sys::default_table();

    // -O2 makes these asserts pass
    ASSERT(table[SYS_SIGACT] == (void*) sys_sigact);
    ASSERT(table[SYS_SIGATHRD] == (void*) sys_sigathrd);
    ASSERT(table[SYS_PANIC] == (void*) sys_panic);

    // print_all();
}

void module_exit() {
    //
}

void install_handler() {
    constexpr uint64_t kernel_segment = 0x0008ul;
    constexpr uint64_t user_segment   = 0x0013ul;

    Sys::CPU::wrmsr(IA32_STAR, (Sys::CPU::rdmsr(IA32_STAR) & 0xFFFFFFFF) | (kernel_segment << 32) |
                                   (user_segment << 48));
    Sys::CPU::wrmsr(IA32_LSTAR, (void*) handler);
}

void register_syscalls() {
    auto table = Sys::default_table();

    register_fs(table);
    register_ipc(table);
    register_proc(table);
    register_sec(table);
    register_net(table);
    register_test(table);
}

extern "C" void syscall_prepare(int syscall) {
    ASSERT(!Proc::Thread::current()->isCritical());
}

extern "C" void syscall_done(int syscall, IPC::syscall_registers* regs) {
    auto thread = Proc::Thread::current();
    ASSERT(!thread->isCritical());

    thread->sigchk(regs);
    thread->abortchk();
}

void print_all() {
    for (size_t i = 0; i < 256; i++) {
        auto aaa = Debug::addr2name(Sys::default_table()[i]);
        printk("%4i. %s\n", i, aaa ?: "unknown");
    }
}