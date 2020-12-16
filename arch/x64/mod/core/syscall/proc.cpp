#include "aex/errno.hpp"
#include "aex/proc/process.hpp"
#include "aex/proc/thread.hpp"
#include "aex/sys/syscall.hpp"

#include "syscallids.h"

using namespace AEX;
using namespace AEX::Proc;

namespace AEX::Proc {
    void add_thread(Thread* thread);
    void remove_thread(Thread* thread);
}

void exit(int status) {
    Process::current()->exit(status);
    Thread::current()->errno = ENONE;
}

void thexit() {
    Thread::exit();
    Thread::current()->errno = ENONE;
}

void usleep(int ns) {
    Thread::usleep(ns);
    Thread::current()->errno = ENONE;
}

void yield() {
    Thread::yield();
    Thread::current()->errno = ENONE;
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

    if (parent->mmap_regions.count() > 0)
        kpanic("implement proper mmap forking\n");

    auto calling_thread = Thread::current();
    auto thread         = new Thread();

    thread->status = calling_thread->status;
    thread->parent = child;

    thread->context = new Context();

    Sys::CPU::nointerrupts();
    memcpy(thread->context, calling_thread->context, sizeof(Context));
    Sys::CPU::interrupts();

    thread->user_stack      = calling_thread->user_stack;
    thread->user_stack_size = calling_thread->user_stack_size;

    thread->kernel_stack =
        (size_t) Mem::kernel_pagemap->alloc(calling_thread->kernel_stack_size, PAGE_WRITE);
    thread->kernel_stack_size = calling_thread->kernel_stack_size;

    thread->fault_stack = (size_t) Mem::kernel_pagemap->alloc(Thread::FAULT_STACK_SIZE, PAGE_WRITE);
    thread->fault_stack_size = Thread::FAULT_STACK_SIZE;

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
    thread->context->cr3    = child->pagemap->pageRoot;

    Mem::atomic_add(&child->thread_counter, 1);

    child->lock.acquire();
    child->threads.push(thread);
    child->lock.release();

    child->ready();

    add_thread(thread);

    return child->pid;
}

void register_proc() {
    auto table = Sys::default_table();

    table[SYS_EXIT]   = (void*) exit;
    table[SYS_THEXIT] = (void*) thexit;
    table[SYS_USLEEP] = (void*) usleep;
    table[SYS_YIELD]  = (void*) yield;
    table[SYS_FORK]   = (void*) fork;
}