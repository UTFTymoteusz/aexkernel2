#include "aex/errno.hpp"
#include "aex/mem/usr.hpp"
#include "aex/proc/exec.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"
#include "usr.hpp"

using namespace AEX;
using namespace AEX::Mem;
using namespace AEX::Proc;

void register_proc_thread(Sys::syscall_t* table);

void sys_exit(int status) {
    Process::current()->exit(status);
    USR_ERRNO = ENONE;
}

pid_t sys_fork() {
    auto parent = Process::current();
    auto child =
        new Process(parent->image_path, parent->pid, parent->pagemap->fork(), parent->name);

    child->session = parent->session;
    child->group   = parent->group;

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

    thread->user_stack.ptr  = calling_thread->user_stack.ptr;
    thread->user_stack.size = calling_thread->user_stack.size;

    thread->kernel_stack.ptr =
        Mem::kernel_pagemap->alloc(calling_thread->kernel_stack.size, PAGE_WRITE);
    thread->kernel_stack.size = calling_thread->kernel_stack.size;

    thread->fault_stack.ptr  = Mem::kernel_pagemap->alloc(Thread::FAULT_STACK_SIZE, PAGE_WRITE);
    thread->fault_stack.size = Thread::FAULT_STACK_SIZE;

    auto stack_end = (size_t*) ((size_t) calling_thread->kernel_stack.ptr +
                                calling_thread->kernel_stack.size - 512 - 32);

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

    printkd(PTKD_EXEC, "syscall: pid%i: forked as pid%i\n", parent->pid, child->pid);

    add_thread(thread);

    return child->pid;
}

optional<int> usr_get_tblc(usr_char* const argv[]) {
    int argc = 0;

    for (int i = 0; i < 32; i++) {
        auto read_try = u_read<usr_char*>(&argv[i]);
        USR_ENSURE(read_try);

        if (read_try.value == nullptr)
            break;

        argc++;
    }

    return argc;
}

bool usr_get_tbl(usr_char* const argv[], int argc, tmp_array<char>* argv_buff) {
    size_t arg_len = 0;

    for (int i = 0; i < argc; i++) {
        auto strlen_try = u_strlen(argv[i]);
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

int sys_execve(const usr_char* path, usr_char* const usr_argv[], usr_char* const usr_envp[]) {
    char path_buffer[FS::PATH_MAX];
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

    printkd(PTKD_EXEC, "proc: pid%i: exec '%s'\n", Process::current()->pid, path_buffer);

    USR_ERRNO = exec(Process::current(), Thread::current(), path_buffer, argv,
                     usr_envp ? envp : nullptr, nullptr);

    return USR_ERRNO != ENONE ? -1 : 0;
}

pid_t sys_wait(usr_int* wstatus) {
    SCOPE(Thread::current()->signabilityGuard);

    int   status;
    pid_t pid = USR_ENSURE_OPT(Process::current()->wait(status));

    USR_ENSURE_OPT(u_write(wstatus, status));
    return pid;
}

pid_t sys_getpid() {
    return Process::current()->pid;
}

tid_t sys_gettid() {
    return Thread::current()->tid;
}

int sys_nice(int nice) {
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

int sys_getenv(int index, usr_char* buffer, size_t len) {
    auto current = Process::current();
    auto scope   = current->lock.scope();

    char* line = USR_ENSURE_OPT(current->envGet(index));

    USR_ENSURE_R(strlen(line) + 1 <= len, ERANGE);
    USR_ENSURE_OPT(k2u_memcpy(buffer, line, strlen(line) + 1));

    return 0;
}

pid_t sys_setsid() {
    auto current = Process::current();
    if (current->isGroupLeader()) {
        USR_ERRNO = EPERM;
        return -1;
    }

    current->session = current->pid;
    current->group   = current->pid;

    USR_ERRNO = ENONE;
    return current->pid;
}

O2 void register_proc(Sys::syscall_t* table) {
    table[SYS_EXIT]   = (void*) sys_exit;
    table[SYS_FORK]   = (void*) sys_fork;
    table[SYS_EXECVE] = (void*) sys_execve;
    table[SYS_WAIT]   = (void*) sys_wait;
    table[SYS_GETPID] = (void*) sys_getpid;
    table[SYS_GETTID] = (void*) sys_gettid;
    table[SYS_NICE]   = (void*) sys_nice;
    table[SYS_GETENV] = (void*) sys_getenv;
    table[SYS_SETSID] = (void*) sys_setsid;

    register_proc_thread(table);
}