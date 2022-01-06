#include "aex/arch/ipc/signal.hpp"

#include "aex/mem/paging.hpp"

namespace AEX::IPC {
    extern "C" void sigret_trampoline_func();

    void* sigret_trampoline;

    void arch_signal_init() {
        sigret_trampoline = Mem::kernel_pagemap->alloc(4096, PAGE_EXEC | PAGE_USER);
        memcpy(sigret_trampoline, (void*) sigret_trampoline_func, 16);
    }
}