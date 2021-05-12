#include "aex/errno.hpp"
#include "aex/proc/exec.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Proc;

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

    for (int i = 0; i < parent->descs.count(); i++) {
        if (!parent->descs.present(i))
            continue;

        auto desc = parent->descs.at(i);
        child->descs.set(i, {desc.file, desc.flags});
    }

    for (int i = 0; i < parent->mmap_regions.count(); i++) {
        if (!parent->mmap_regions.present(i))
            continue;

        auto fork_try = parent->mmap_regions.at(i)->fork(child->pagemap);
        if (!fork_try.has_value)
            BROKEN;

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
    thread->context->cr3    = child->pagemap->root;

    Mem::atomic_add(&child->thread_counter, 1);

    parent->lock.acquire();
    child->env(&parent->env());
    parent->lock.release();

    child->set_cwd(parent->get_cwd());
    child->assoc(thread);
    child->ready();

    printkd(PTK_DEBUG, "syscall: pid%i: forked as pid%i\n", parent->pid, child->pid);

    add_thread(thread);

    return child->pid;
}

optional<int> usr_get_tblc(usr_char* const argv[]) {
    int argc = 0;

    for (int i = 0; i < 32; i++) {
        auto read_try = usr_read<usr_char*>(&argv[i]);
        USR_ENSURE(read_try);

        if (read_try.value == nullptr)
            break;

        argc++;
    }

    return argc;
}

bool usr_get_tbl(usr_char* const argv[], int argc, tmp_array<char>* argv_buff) {
    int arg_len = 0;

    for (size_t i = 0; i < argc; i++) {
        auto strlen_try = usr_strlen(argv[i]);
        if (!strlen_try)
            return false;

        arg_len += strlen_try.value + 1;
        if (arg_len + 1 > ARG_MAX)
            return false;

        argv_buff[i].resize(strlen_try.value + 1);
        if (!u2k_memcpy(argv_buff[i].get(), argv[i], strlen_try.value + 1))
            return false;
    }

    return true;
}

void usr_finalize_tbl(tmp_array<char>* argv_buff, int argc, char** table) {
    for (int i = 0; i < argc; i++)
        table[i] = argv_buff[i].get();

    table[argc] = nullptr;
}

int execve(const usr_char* path, usr_char* const usr_argv[], usr_char* const usr_envp[]) {
    char path_buffer[FS::MAX_PATH_LEN];
    USR_ENSURE(copy_and_canonize(path_buffer, path));

    int argc = USR_ENSURE(usr_get_tblc(usr_argv));
    USR_ENSURE(argc > 0 && argc <= ARGC_MAX);

    tmp_array<char> argv_buffers[argc];
    char*           argv[argc + 1];

    USR_ENSURE(usr_get_tbl(usr_argv, argc, argv_buffers));
    usr_finalize_tbl(argv_buffers, argc, argv);

    int envc = 1;

    if (usr_envp) {
        envc = USR_ENSURE(usr_get_tblc(usr_envp));
        USR_ENSURE(envc > 0 && envc <= ENVC_MAX);
    }

    tmp_array<char> envp_buffers[envc];
    char*           envp[envc + 1];

    if (usr_envp) {
        USR_ENSURE(usr_get_tbl(usr_envp, envc, envp_buffers));
        usr_finalize_tbl(envp_buffers, envc, envp);
    }

    printkd(PTK_DEBUG, "proc: pid%i: exec '%s'\n", Process::current()->pid, path_buffer);

    USR_ERRNO = exec(Process::current(), Thread::current(), path_buffer, argv,
                     usr_envp ? envp : nullptr, nullptr);

    return USR_ERRNO != ENONE ? -1 : 0;
}

pid_t wait(usr_int* wstatus) {
    return USR_ENSURE_OPT(Process::current()->wait(*wstatus));
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

int getenv(int index, usr_char* buffer, size_t len) {
    auto current = Process::current();
    auto scope   = current->lock.scope();

    char* line = USR_ENSURE_OPT(current->envGet(index));

    USR_ENSURE_R(strlen(line) + 1 <= len, ERANGE);
    USR_ENSURE_OPT(k2u_memcpy(buffer, line, strlen(line) + 1));

    return 0;
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
    table[SYS_GETENV] = (void*) getenv;
}