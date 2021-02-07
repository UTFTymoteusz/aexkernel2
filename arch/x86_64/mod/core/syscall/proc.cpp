#include "aex/errno.hpp"
#include "aex/proc/exec.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Proc;

namespace AEX::Proc {
    void add_thread(Thread* thread);
    void remove_thread(Thread* thread);
}

void exit(int status) {
    Process::current()->exit(status);
    USR_ERRNO = ENONE;
}

void thexit() {
    Thread::exit();
    USR_ERRNO = ENONE;
}

void usleep(uint64_t ns) {
    Thread::usleep(ns);
    USR_ERRNO = ENONE;
}

void yield() {
    Thread::yield();
    USR_ERRNO = ENONE;
}

pid_t fork() {
    auto parent = Process::current();
    auto child =
        new Process(parent->image_path, parent->pid, parent->pagemap->fork(), parent->name);

    for (int i = 0; i < parent->files.count(); i++) {
        if (!parent->files.present(i))
            continue;

        child->files.set(i, parent->files.at(i)->dup().value);
    }

    for (int i = 0; i < parent->mmap_regions.count(); i++) {
        if (!parent->mmap_regions.present(i))
            continue;

        auto fork_try = parent->mmap_regions.at(i)->fork(child->pagemap);
        if (!fork_try.has_value)
            kpanic("it's broken");

        child->mmap_regions.push(fork_try.value);
    }

    auto calling_thread = Thread::current();
    auto thread         = new Thread();

    thread->status = calling_thread->status;
    thread->parent = child;

    // TODO: Make all this be inside of the Thread class

    thread->context = new Context();
    memcpy(thread->context, calling_thread->context, sizeof(Context));

    thread->context_aux = new Context();

    thread->user_stack      = calling_thread->user_stack;
    thread->user_stack_size = calling_thread->user_stack_size;

    thread->kernel_stack =
        (size_t) Mem::kernel_pagemap->alloc(calling_thread->kernel_stack_size, PAGE_WRITE);
    thread->kernel_stack_size = calling_thread->kernel_stack_size;

    thread->fault_stack = (size_t) Mem::kernel_pagemap->alloc(Thread::FAULT_STACK_SIZE, PAGE_WRITE);
    thread->fault_stack_size = Thread::FAULT_STACK_SIZE;

    thread->aux_stack = (size_t) Mem::kernel_pagemap->alloc(Thread::AUX_STACK_SIZE, PAGE_WRITE);
    thread->aux_stack_size = Thread::AUX_STACK_SIZE;

    auto stack_end =
        (size_t*) (calling_thread->kernel_stack + calling_thread->kernel_stack_size - 32);

    thread->tls = calling_thread->tls;

    thread->context->rflags = stack_end[0];
    thread->context->rip    = stack_end[1];
    thread->context->rbp    = stack_end[2];
    thread->context->rsp    = stack_end[3];
    thread->context->cs     = 0x23;
    thread->context->ss     = 0x1B;
    thread->context->rax    = 0;
    thread->context->rdx    = 0;
    thread->context->r12    = 0;
    thread->context->cr3    = child->pagemap->pageRoot;

    Mem::atomic_add(&child->thread_counter, 1);

    child->set_cwd(parent->get_cwd());
    child->assoc(thread);
    child->ready();

    PRINTK_DEBUG2("pid%i: forked as pid%i", parent->pid, child->pid);

    add_thread(thread);

    return child->pid;
}

optional<int> usr_get_argc(usr_char* const argv[]) {
    int argc = 0;

    for (int i = 0; i < 32; i++) {
        auto read_try = usr_read<usr_char*>(&argv[i]);
        if (!read_try) {
            USR_ERRNO = EINVAL;
            return -1;
        }

        if (read_try.value == nullptr)
            break;

        argc++;
    }

    return argc;
}

int execve(const usr_char* path, usr_char* const argv[], usr_char* const envp[]) {
    auto strlen_try = usr_strlen(path);
    if (!strlen_try) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    char path_buffer[FS::MAX_PATH_LEN];
    if (!copy_and_canonize(path_buffer, path)) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    auto argc_try = usr_get_argc(argv);
    if (!argc_try.value) {
        USR_ERRNO = EINVAL;
        return -1;
    }

    int argc = argc_try.value;

    tmp_array<char> tmp_buffers[argc];
    char*           argv_buffer[argc + 1];

    for (size_t i = 0; i < argc; i++) {
        auto strlen_try = usr_strlen(argv[i]);
        if (!strlen_try)
            return EINVAL;

        tmp_buffers[i].resize(strlen_try.value + 1);
        argv_buffer[i] = tmp_buffers[i].get();

        if (!u2k_memcpy(argv_buffer[i], argv[i], strlen_try.value + 1))
            return EINVAL;
    }

    argv_buffer[argc] = nullptr;

    PRINTK_DEBUG2("pid%i: exec '%s'", Process::current()->pid, path_buffer);

    USR_ERRNO =
        exec(Process::current(), Thread::current(), path_buffer, argv_buffer, envp, nullptr);

    return USR_ERRNO != ENONE ? -1 : 0;
}

pid_t wait(usr_int* wstatus) {
    auto result = Process::current()->wait(*wstatus);
    if (!result) {
        USR_ERRNO = result.error_code;
        return -1;
    }

    return result.value;
}

pid_t getpid() {
    return Process::current()->pid;
}

int nice(int nice) {
    auto current = Process::current();

    if (nice < 0 && current->eff_uid != 0) {
        USR_ERRNO = EPERM;
        return -1;
    }

    if (!inrange(nice, -19, 20)) {
        USR_ERRNO = EPERM;
        return -1;
    }

    current->nice = nice;

    return nice;
}

void register_proc() {
    auto table = Sys::default_table();

    table[SYS_EXIT]   = (void*) exit;
    table[SYS_THEXIT] = (void*) thexit;
    table[SYS_USLEEP] = (void*) usleep;
    table[SYS_YIELD]  = (void*) yield;
    table[SYS_FORK]   = (void*) fork;
    table[SYS_EXECVE] = (void*) execve;
    table[SYS_WAIT]   = (void*) wait;
    table[SYS_GETPID] = (void*) getpid;
    table[SYS_NICE]   = (void*) nice;
}