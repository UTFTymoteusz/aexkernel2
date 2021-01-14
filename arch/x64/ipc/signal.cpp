#include "aex/arch/ipc/signal.hpp"

#include "aex/arch/mem/helpers/usr_stack.hpp"
#include "aex/mem/paging.hpp"

namespace AEX::IPC {
    extern "C" void sigret_trampoline_func();

    void* sigret_trampoline;

    void arch_signal_init() {
        sigret_trampoline = Mem::kernel_pagemap->alloc(4096, PAGE_EXEC | PAGE_USER);
        memcpy(sigret_trampoline, (void*) sigret_trampoline_func, 16);
    }

    error_t signal_context(Proc::Thread* thread, uint8_t id, sigaction& action) {
        auto stack = Mem::usr_stack(thread, thread->saved_stack);

        stack.push(action.flags & SA_RESTORER ? action.restorer : sigret_trampoline);

        thread->context->rip = (size_t) action.handler;
        thread->context->rdi = id;

        thread->context->rsp = stack.pointer();
        thread->context->cs  = 0x23;
        thread->context->ss  = 0x1B;

        return ENONE;
    }

    error_t signal_context(Proc::Thread* thread, uint8_t id, sigaction& action, siginfo_t& info) {
        auto stack = Mem::usr_stack(thread, thread->saved_stack);

        stack.push(info);
        auto siginfo_rsp = stack.pointer();

        stack.push(action.flags & SA_RESTORER ? action.restorer : sigret_trampoline);

        thread->context->rip = (size_t) action.handler;
        thread->context->rdi = id;
        thread->context->rsi = siginfo_rsp;

        thread->context->rsp = stack.pointer();
        thread->context->cs  = 0x23;
        thread->context->ss  = 0x1B;

        return ENONE;
    }
}